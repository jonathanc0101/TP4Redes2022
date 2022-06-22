/**
 * Servidor echo UDP y TCP
 * -----------------------
 *
 * El servidor escucha en el IP:PUERTO indicado como parámetro en la línea de
 * comando, o 127.0.0.1:8888 en caso de que no se indique una dirección, tanto
 * paquetes UDP como conexiones TCP.
 *
 * Al recibir un datagrama UDP imprime el contenido del mismo, precedido de la
 * dirección del emisor.
 *
 * Para terminar la ejecución del servidor, envíar una señal SIGTERM (^C)
 *
 * Se puede probar el funcionamiento del servidor con el programa netcat:
 *
 * nc -u 127.0.0.1 8888
 *
 *
 * ---
 * Autor: Francisco Paez
 * Fecha: 2022-06-03
 * Última modificacion: 2022-06-03
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <signal.h>
#include <pthread.h>

#include "estructuras.h"
#include "estructuras_cliente.h"
#include "estructuras.c"

// Dirección por defecto del servidor.
#define PORT 9898
#define IP "127.0.0.1"

#define FILEBUFSIZE 1024

typedef struct Usuario
{
    char nombre[24];
    int id;
    int conectado;
    struct sockaddr_in addr;
    int fd;
} Usuario;

static Usuario *users;
static int registeredUsers = 0;

// mutex global
pthread_mutex_t lock;

// Tamaño del buffer en donde se reciben los mensajes.
#define BUFSIZE 300

// Máximo número de conexiones pendientes en el socket TCP.
#define PENDING 10

// Maximo numero de usuarios registrados
#define MAX_USERS 100

// Cierra el socket al recibir una señal SIGTERM.
void handler(int signal)
{
    exit(EXIT_SUCCESS);
}

void miPerror(char nombre[], int valor)
{
    if (valor < 0)
    {
        perror(nombre);
        exit(EXIT_FAILURE);
    }
}

void imprimirUsuarios(){
    if(registeredUsers < 1){
        printf("no hay usuarios registrados");
        return;
    }
    else{
        printf("[%d] usuarios registrados\n", registeredUsers);
    }

    for(int i = 1; i <= registeredUsers; i++){
        printf("usuario %d nombre %s\n", users[i].id ,users[i].nombre);
    }
}

struct_cliente_UDPS buscarUsuarioPorNombreInseguro(struct_cliente_UDPS paquete){

    paquete.codigo = -2;

    for(int i = 1; i <= registeredUsers; i++){
        if(memcmp(users[i].nombre, paquete.userName, sizeof(paquete.userName)) == 0){

            paquete.id = users[i].id;

            if(users[i].conectado){
                paquete.codigo = 0;
                
            }else{
                paquete.codigo = -1;
            }

            return paquete;
        }
    }

    return paquete;
}

void buscarUsuario(char buf[], int sock,struct sockaddr_in src_addr, socklen_t src_addr_len){
    // bloqueamos el mutex para evitar condiciones de carrera
    pthread_mutex_lock(&lock);

    // src_addr.sin_family = AF_INET;

    printf("buscando usuario, mutex bloqueado\n");

    // estructura a ser enviada al usuario
    struct_cliente_UDPS paquete;
    struct_cliente_UDPS paqueteNuevo;

    paquete = unpackUDPS(buf);
    
    paqueteNuevo = buscarUsuarioPorNombreInseguro(paquete);

    printf("Nombre del usuario a buscar: [%s] \n", paquete.userName);
    
    // enviamos respuesta
    int s = sendto(sock, &paqueteNuevo, sizeof(paqueteNuevo), 0, (struct sockaddr *)&src_addr, sizeof(src_addr));
    miPerror("sendto", s);

    imprimirUsuarios();

    // desbloqueamos el mutex para que otro hilo lo pueda usar
    printf("usuario buscado, mutex desbloqueado\n");
    pthread_mutex_unlock(&lock);
}

void registrarUsuario(char buf[], int sock,struct sockaddr_in src_addr, socklen_t src_addr_len)
{
    // bloqueamos el mutex para evitar condiciones de carrera
    pthread_mutex_lock(&lock);

    src_addr.sin_family = AF_INET;

    printf("registrando usuario, mutex bloqueado\n");

    registeredUsers++;

    Usuario nuevoUsuario;

    // estructura a ser enviada al usuario
    struct_cliente_UDPR miStruct;

    miStruct = unpackUDPR(buf);
    
    miStruct.codigo = 0;
    miStruct.id = registeredUsers;

    printf("Datos del usuario a registrarse: [NOMBRE] %s  [ID] %d\n", miStruct.userName,miStruct.id);
    
    // persistencia en el servidor
    memcpy(&nuevoUsuario.nombre,&miStruct.userName,sizeof(miStruct.userName));

    nuevoUsuario.conectado = 0;
    nuevoUsuario.id = registeredUsers;

    char *nuevoBuf;
    nuevoBuf = (char*)&miStruct;

    // enviamos respuesta
    int s = sendto(sock, nuevoBuf, sizeof(miStruct), 0, (struct sockaddr *)&src_addr, sizeof(src_addr));
    miPerror("sendto", s);
    

    //persistimos en el array
    users[registeredUsers] = nuevoUsuario;

    imprimirUsuarios();

    // desbloqueamos el mutex para que otro hilo lo pueda usar
    printf("usuario registrado, mutex desbloqueado\n");
    pthread_mutex_unlock(&lock);

}

void errorEnLogearUsuario(struct_cliente_TCPL paquete){
    paquete.codigo = -1;
    users[paquete.id].conectado = 0;
    pthread_mutex_unlock(&lock);
}

int loguearUsuario(int socket,  struct_cliente_TCPL usuario)
{   
    // desbloqueamos el mutex para que otro hilo lo pueda usar
    pthread_mutex_lock(&lock);

    int id = usuario.id;
    char nombre[24];
    memcpy(nombre,usuario.userName,sizeof(usuario.userName));
    nombre[23] = '\0';
    
    printf("ID usuario a loguear: %d, usuarios registrados %d nombre usuario %s\n", id, registeredUsers, nombre);

    usuario.codigo = 0;

    if (id <= 0 || id > registeredUsers)
    {
        printf("El id de usuario no existe: [id: %d] \n", id);
        
        errorEnLogearUsuario(usuario);
        return -1;
    }

    if(users[id].conectado){
        printf("El usuario ya está conectado: [id: %d] \n", id);
        
        errorEnLogearUsuario(usuario);
        return -1;
    }else{
        users[id].conectado = 1;
    }

    if (strcmp(users[id].nombre, nombre) != 0)
    {
        printf("Nombre de usuario incorrecto: %s\n", nombre);
        
        errorEnLogearUsuario(usuario);
        return -1;
    }

    printf("Usuario logeado [ID:%d] [Nombre:%s] conectado\n", usuario.id, usuario.userName);

    socklen_t len; 
    int g = getpeername(socket,(struct sockaddr *)&users[id].addr, &len);
    miPerror("getpeername",g);

    users[id].fd = socket;

    printf("[%s:%d][fd:%d][añadido a los usuarios loggeados]\n", inet_ntoa(users[id].addr.sin_addr), ntohs(users[id].addr.sin_port),users[id].fd);

    char *nuevoBuf = (char*)&usuario;

    int n = write(socket,nuevoBuf,sizeof(usuario));
    miPerror("write",n);

    // desbloqueamos el mutex para que otro hilo lo pueda usar
    pthread_mutex_unlock(&lock);

    return id;
}

int handleLoguearUsuario(int socket)
{
    struct_TCPL usuario;
    struct_cliente_TCPL paquete;
    memset(&usuario,0,sizeof(usuario));
    memset(&paquete,0,sizeof(paquete));

    memcpy(paquete.codop, "TCPL", sizeof("TCPL"));

    int n = read(socket, &usuario, sizeof(usuario));
    miPerror("read", n);

    paquete.codigo = 0;
    memcpy(paquete.userName, usuario.
    
    userName,sizeof(paquete.userName ));
    paquete.id = usuario.id;

    return loguearUsuario(socket, paquete);
}
void handleUDP(int socket_udp, struct sockaddr_in src_addr, socklen_t src_addr_len)
{
    char buf[BUFSIZE];
    char codigo[5];
    memset(&buf, 0, sizeof(buf));        
    memset(&codigo, 0, sizeof(codigo));        

    struct sockaddr_in cli_addr;
    memset(&cli_addr, 0, sizeof(cli_addr));        

    socklen_t cli_addr_len;

    // Recibe un mensaje entrante.
    ssize_t n = recvfrom(socket_udp, buf, BUFSIZE, 0, (struct sockaddr *)&cli_addr, &cli_addr_len);
    miPerror("recvfrom", n);

    memcpy(&codigo,&buf,sizeof(codigo)-1);
    codigo[4] = '\0';
    
    // Imprime dirección del emisor y mensaje recibido.
    printf("[%s:%d][UDP] [%s][CODOP]\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), codigo);

    if(strcmp(codigo, "UDPR\0") == 0){
        registrarUsuario(buf, socket_udp, cli_addr, cli_addr_len);
    }else if(strcmp(codigo,"UDPS\0") == 0){
        buscarUsuario(buf,socket_udp,cli_addr,cli_addr_len);
    }


}

int existeIDInseguro(int id){
    int existeID =  id <= registeredUsers && id > 0;

    return existeID ;
}

int existeID(int id){
    pthread_mutex_lock(&lock);
    int existeID =  id <= registeredUsers && id > 0;
    pthread_mutex_unlock(&lock);

    return existeID ;
}


void errorEnviarMensaje(struct_cliente_TMSJ paquete){
    paquete.codigo = -1;
        
    int n = write(users[paquete.userIDSender].fd,&paquete,sizeof(paquete));
    miPerror("write",n);

    pthread_mutex_unlock(&lock);
    return;
}

void enviarMensaje(int sock, struct_cliente_TMSJ  paquete){
    int n ;

    pthread_mutex_lock(&lock);

    printf("enviando mensaje\n");
    printf("useridsender[%d]\n",paquete.userIDSender);

    // vemos que los usuarios existan
    if(existeIDInseguro(paquete.userIDSender) != 1){

        printf("Sender no existe: [%d]\n mensaje no enviado\n", paquete.userIDSender);

        errorEnviarMensaje(paquete);
        return;
    } 

    if(existeIDInseguro(paquete.userIDReceiver) != 1){

        printf("Receiver no existe: [%d]\n mensaje no enviado\n", paquete.userIDReceiver);

        errorEnviarMensaje(paquete);
        return;
    }     

    // vemos que esten conectados
    if(users[paquete.userIDReceiver].conectado == 0){
        printf("Receiver no conectado: [%d]\n mensaje no enviado\n", paquete.userIDReceiver);

        errorEnviarMensaje(paquete);
        return;
    }

    if(users[paquete.userIDSender].conectado == 0){
        printf("Sender no conectado: [%d]\n mensaje no enviado\n", paquete.userIDReceiver);

        errorEnviarMensaje(paquete);
        return;
    }

    Usuario usuario2 = users[paquete.userIDReceiver];

    //enviamos el mensaje
    paquete.codigo = 0;
    n = write(usuario2.fd, &paquete,sizeof(paquete));
    miPerror("write",n);

    // notificamos al usuario que está todo bien
    struct_cliente_TMSR respuesta;
    memset(&respuesta,0,sizeof(respuesta));

    memcpy(respuesta.codop,"TMSR",sizeof("TMSR"));
    respuesta.codigo = 0;
    respuesta.userIDSender = paquete.userIDSender;

    n = write(users[paquete.userIDSender].fd, &respuesta,sizeof(respuesta));
    miPerror("write",n);

    pthread_mutex_unlock(&lock);

    printf("mensaje enviado\n");
}

void handleSolicitudMensaje(int sock){
    struct_TMSJ paqueteRecibido;
    struct_cliente_TMSJ paquete;
    memset(&paqueteRecibido,0,sizeof(paqueteRecibido));
    memset(&paquete,0,sizeof(paquete));

    memcpy(paquete.codop, "TMSJ", sizeof("TMSJ"));

    int n = read(sock, &paqueteRecibido, sizeof(paqueteRecibido));
    miPerror("read", n);

    printf("mensaje a enviar: %s\n", paqueteRecibido.msj);

    //copiamos los paquetes
    memcpy(paquete.msj,paqueteRecibido.msj, sizeof(paqueteRecibido.msj));
    paquete.codigo = 0;
    paquete.userIDSender = paqueteRecibido.userIDSender;
    paquete.userIDReceiver = paqueteRecibido.userIDReceiver;

    enviarMensaje(sock, paquete);
}

void cerrarSesion(int id){
    pthread_mutex_lock(&lock);

    users[id].conectado = 0;
    memset(&users[id].addr,0,sizeof(users[id].addr));
    users[id].fd= -1;

    pthread_mutex_unlock(&lock);
}

void entubarNBytes(int fd1, int fd2, long int n){
    char buf[FILEBUFSIZE];
    ssize_t numRead;

    printf("total de bytes [%ld]\n",n);

    for(long int totRead = 0; totRead<n;){
        printf("entubando %ld de %ld bytes\n", totRead, n);

        memset(&buf,0,FILEBUFSIZE);
        int delta = n - totRead;
        
        if(FILEBUFSIZE <= delta){
            numRead = read(fd1, buf, FILEBUFSIZE);
        }else{
            numRead = read(fd1, buf, delta);
        }
        miPerror("read",n);

        if(numRead>0){
            totRead+=numRead;
            int m = write(fd2,buf,numRead);
            miPerror("write",m);
        }

        // printf("entubando %ld de %ld bytes\n", totRead, n);
    }

    printf("Archivo entubado correctamente\n");
}

void errorEnviarArchivo(struct_cliente_TCFS paquete){
    paquete.codigo = -1;
    int fd = users[paquete.userIDSender].fd;

    int n = write(fd,&paquete,sizeof(paquete));
    miPerror("write",n);

    pthread_mutex_unlock(&lock);
}

void enviarArchivo(int fd, struct_cliente_TCFS paquete){
    int n ;

    pthread_mutex_lock(&lock);

    printf("enviando archivo\n");
    printf("useridsender[%d]\n",paquete.userIDSender);
    printf("useridreceiver[%d]\n",paquete.userIDReceiver);

    // vemos que los usuarios existan
    if(existeIDInseguro(paquete.userIDSender) != 1){

        printf("Sender no existe: [%d]\n archivo no enviado\n", paquete.userIDSender);

        errorEnviarArchivo(paquete);
        return;
    } 

    if(existeIDInseguro(paquete.userIDReceiver) != 1){

        printf("Receiver no existe: [%d]\n archivo no enviado\n", paquete.userIDReceiver);

        errorEnviarArchivo(paquete);
        return;
    }     

    // vemos que esten conectados
    if(users[paquete.userIDReceiver].conectado == 0){
        printf("Receiver no conectado: [%d]\n archivo no enviado\n", paquete.userIDReceiver);

        errorEnviarArchivo(paquete);
        return;
    }

    if(users[paquete.userIDSender].conectado == 0){
        printf("Sender no conectado: [%d]\n archivo no enviado\n", paquete.userIDReceiver);

        errorEnviarArchivo(paquete);
        return;
    }

    Usuario usuario1 = users[paquete.userIDSender];
    Usuario usuario2 = users[paquete.userIDReceiver];

    //notificamos al usuario 1 para que sepa que puede arrancar a transmitir el archivo
    paquete.codigo = 0;
    n = write(usuario1.fd, &paquete,sizeof(paquete));
    miPerror("write",n);

    // notificamos al usuario 2 para que comience a leer el archivo
    n = write(usuario2.fd, &paquete,sizeof(paquete));
    miPerror("write",n);

    printf("fd1[%d] fd2[%d] a entubar \n", usuario1.fd,usuario2.fd);

    entubarNBytes(usuario1.fd,usuario2.fd,paquete.fileSize);

    pthread_mutex_unlock(&lock);

    printf("archivo enviado\n");
}

void handleSolicitudArchivo(int sock){
    struct_cliente_TCFS paquete;
    memset(&paquete,0,sizeof(paquete));

    memcpy(paquete.codop, "TCFS", sizeof("TCFS"));

    int n = read(sock, &paquete.codigo, sizeof(paquete) - 4);
    miPerror("read", n);

    printf("archivo a enviar: %s\n", paquete.fileName);
    printf("usuario al que se le envía: [ID:%d] \n", paquete.userIDReceiver);


    enviarArchivo(sock, paquete);
}

void atenderSesion(int sock, int id){
    char codop[5];
    memset(&codop, 0, sizeof(codop));

    int n;
    
    while(strcmp(codop,"TCPE\0") != 0){
        memset(&codop, 0, sizeof(codop));

        n = read(sock, codop, 4);
        miPerror("read", n);

        codop[4] = '\0';

        if(strcmp(codop,"TMSJ\0") == 0){
            // mensaje
            handleSolicitudMensaje(sock);
        }else if(strcmp(codop,"TCFS\0") == 0){
            // se envía archivo a usuario
            handleSolicitudArchivo(sock);
        }
        else if(strcmp(codop,"TCPE\0")){
            // cerrar sesion
            cerrarSesion(id);
        }
    }

}

static void* handleTCP(void *arg)
{      
    int sock = *((int *) arg);

    printf("manejando TCP [FD %d]\n",sock); 

    int n; 

    // en base a este codop decimos que hacer
    char codop[5];
    memset(&codop, 0, sizeof(codop));

    n = read(sock, codop, 4);
    miPerror("read", n);

    codop[4] = '\0';
        
    if(strcmp(codop,"TCPL\0") == 0){        
        int id = handleLoguearUsuario(sock);
        
        if(id > 0){
            atenderSesion(sock, id);
        }else{
            printf("sesion no iniciada, error [atenderSesion:%d]\n",id);
        }
    }
        
    pthread_exit(NULL);
}

void *loopTCP(void *arg)
{      
    int socket_tcp = *((int *) arg);
    pthread_t hiloTCP;

    struct sockaddr_in src_addr;
    socklen_t src_addr_len;
    src_addr_len = sizeof(struct sockaddr_in);
    int sock;

    while(1){
        memset(&src_addr, 0, sizeof(struct sockaddr_in));

        // Existe una conexión entrante en el socket TCP.
        sock = accept(socket_tcp, (struct sockaddr *)&src_addr, &src_addr_len);
        miPerror("accept", sock);

        pthread_create(&hiloTCP,NULL,&handleTCP,(void*) &sock);
    }
}


int main(int argc, char *argv[])
{
    pthread_t hiloTCP;
    
    if(pthread_mutex_init(&lock, NULL) <0){
        exit(EXIT_FAILURE);
    }

    users = malloc(sizeof(Usuario) * MAX_USERS);
    struct sockaddr_in addr;   

    // Descriptores de archivo de los sockets.
    int socket_udp;
    int socket_tcp;

    // Configura el manejador de señal SIGTERM.
    signal(SIGTERM, handler);

    // Crea los sockets.
    socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    miPerror("socket_udp", socket_udp);

    socket_tcp = socket(AF_INET, SOCK_STREAM, 0);

    miPerror("socket_tcp", socket_tcp);

    // Dirección donde escuchará el servidor con TCP y UDP.
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_aton(IP, &(addr.sin_addr));

    // Permite reutilizar la dirección que se asociará al socket.
    int optval = 1;
    int optname = SO_REUSEADDR | SO_REUSEPORT;
    if (setsockopt(socket_udp, SOL_SOCKET, optname, &optval, sizeof(optval)) == -1)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(socket_tcp, SOL_SOCKET, optname, &optval, sizeof(optval)) == -1)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Asocia los sockets con la dirección indicada. Tradicionalmente esta
    // operación se conoce como "asignar un nombre al socket".
    int b;
    b = bind(socket_udp, (struct sockaddr *)&addr, sizeof(addr));
    miPerror("bind udp", b);

    b = bind(socket_tcp, (struct sockaddr *)&addr, sizeof(addr));
    miPerror("bind tcp", b);

    // Convierte el socket TCP en pasivo.
    listen(socket_tcp, PENDING);

    printf("Escuchando en %s:%d ...\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

    // hilo TCP
    pthread_create(&hiloTCP,NULL,&loopTCP,(void*) &socket_tcp);

    struct sockaddr_in src_addr;
    socklen_t src_addr_len;
    src_addr_len = sizeof(struct sockaddr_in);
    memset(&src_addr, 0, sizeof(struct sockaddr_in));


    while(1){
        // Existe un paquete entrante en el socket UDP.
        handleUDP(socket_udp, src_addr, src_addr_len);
    }

    // Espera por el hilo tcp.
    if ( pthread_join(hiloTCP, NULL) != 0) {
        perror("pthread_join");
    }

    // Cierra los sockets.
    close(socket_udp);
    close(socket_tcp);


    // se destruye el mutex
    pthread_mutex_destroy(&lock);

    exit(EXIT_SUCCESS);
}
