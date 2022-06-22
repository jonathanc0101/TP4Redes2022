#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>


#include "estructuras_cliente.h"
#include "estructuras.c"


#define MAX 80
#define PORT 9898
#define SA struct sockaddr
#define BUFSIZE 300
#define FILEBUFSIZE 1024

static int id;
int sockfd,sockfdUDP;
struct sockaddr_in servaddr;

void miPerror(char nombre[], int valor)
{
    if (valor < 0)
    {
        perror(nombre);
        exit(EXIT_FAILURE);
    }
}

void vaciarStdin(){
    while (getchar() != '\n');
}
void enviarLogin(int fd)
{
    struct_cliente_TCPL paquete;
    bzero(&paquete, sizeof(paquete));

    char codop[4] = "TCPL";
    char userName[24];
    bzero(userName, sizeof(userName));
    strncpy(userName,"JONA\0",5);

    memcpy(paquete.codop,codop,sizeof(codop));
    memcpy(paquete.userName,userName,sizeof(userName));
    paquete.id = id;

    printf("Usuario a logear [ID:%d] [Nombre:%s] conectado\n", paquete.id, paquete.userName);

    int n = write(fd, &paquete, sizeof(paquete));
    miPerror("write",n);

    n = read(fd,&paquete,sizeof(paquete));
    miPerror("read",n);


    printf("Usuario logeado [ID:%d] [Nombre:%s] conectado\n", paquete.id, paquete.userName);

}

void enviarRegister(int fd, struct sockaddr_in servaddr){
    socklen_t servaddr_len;
    servaddr_len = sizeof(servaddr);
    
    char buf[BUFSIZE];
    bzero(&buf, sizeof(buf));

    struct_cliente_UDPR paquete;
    bzero(&paquete, sizeof(paquete));

    char codop[4] = "UDPR";
    char userName[24];
    bzero(userName, sizeof(userName));
    strncpy(userName,"JONA\0",5);
    int id = 1;

    memmove(&paquete.codop,&codop,sizeof(codop));
    memmove(&paquete.userName,&userName,sizeof(userName));
    memmove(&paquete.id, &id, sizeof(id));

    char *bufNuevo ;
    bufNuevo = (char*)&paquete;
    
    int s = sendto(fd, bufNuevo, sizeof(paquete), 0, (struct sockaddr *)&servaddr, servaddr_len);
    miPerror("sendto", s);

    printf("me bloquee antes del recvfrom\n");

    ssize_t n = recvfrom(fd,buf,sizeof(struct_cliente_UDPR),0,NULL, NULL);
    miPerror("recvfrom", n);

    printf("me bloquee despues del recvfrom\n");

    struct_cliente_UDPR miStruct = unpackUDPR(buf);

    printf("Datos del usuario a registrarse: [NOMBRE] %s  [ID] %d\n", miStruct.userName,miStruct.id);
}

void cerrarSesion(int fd){
    int n = write(fd, "TCPE",sizeof("TCPE"));
    miPerror("write",n);

    printf("sesion cerrada");
}

void imprimirMensaje(int fd){    
    struct_cliente_TMSJ paquete;
    
    memset(&paquete,0,sizeof(paquete));
    
    char codigo[4];
    memcpy(codigo,"TMSJ",4);

    int n = read(fd, &paquete, sizeof(paquete));
    miPerror("read",n);

    if(memcmp(codigo,paquete.codop,4) != 0){
        printf("El paquete recibido no es un mensaje\n");
        return;
    }

    printf("[id:%d]%s\n",paquete.userIDSender,paquete.msj);
}

void enviarMensaje(int fd, char msj[], int idUser){    
    struct_cliente_TMSJ paquete;
    struct_cliente_TMSR paqueteRecibido;
    memset(&paquete,0,sizeof(paquete));
    
    char codop[4];
    memcpy(codop,"TMSJ",4);

    memcpy(paquete.codop,codop,sizeof(codop));
    paquete.userIDSender = id;
    paquete.userIDReceiver = idUser; 
    memcpy(paquete.msj,msj,sizeof(paquete.msj));

    int n = write(fd, &paquete, sizeof(paquete));
    miPerror("write",n);

    printf("Mensaje enviado\n");

    n = read(fd,&paqueteRecibido,sizeof(paqueteRecibido));
    miPerror("read",n);


    if(paqueteRecibido.codigo == 0){
        printf("mensaje entregado\n");
    }else{
        printf("mensaje NO entregado\n");
    }
}

void enviarArchivo(int fd, struct_cliente_TCFS paquete){
    struct_cliente_TCFS paqueteRespuesta;
    memset(&paqueteRespuesta,0,sizeof(paqueteRespuesta));

    char buf[FILEBUFSIZE];
    int file = open(paquete.fileName,O_RDWR,0666);
    miPerror("open",file);


    struct stat st;
    int h = fstat(file, &st);
    miPerror("fstat",h);
    
    paquete.fileSize = st.st_size;
    printf("tam archivo [%ld]\n", paquete.fileSize);
    printf("tam archivo [%ld]\n", st.st_size);


    int m = write(fd,&paquete,sizeof(paquete));
    miPerror("write",m);

    m = read(fd,&paqueteRespuesta,sizeof(paqueteRespuesta));
    miPerror("read",m);

    if(memcmp(&paqueteRespuesta,&paquete,sizeof(paquete)) != 0){
        printf("Error desconocido con la solicitud de enviar archivo\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Abriendo archivo en disco...\n");

    int n = 0;
    long int totRead = 0;
    long int fileSize = paquete.fileSize;

    while(totRead < fileSize){
        printf("enviando archivo... [B:%ld]\n",totRead);
        memset(&buf,0,FILEBUFSIZE);
        long int delta = fileSize - totRead;
        
        if(FILEBUFSIZE <= delta){
            n = read(file, buf, FILEBUFSIZE);
        }else{
            n = read(file, buf, delta);
        }
        miPerror("read",n);

        if (n > 0){
            totRead+=n;
            int m = write(fd,buf,n);
            miPerror("write",m);
        }
    }

    close(file);

    printf("Archivo enviado\n");


}

void handleEnviarArchivo(int fd, int userIDReceiver,char fileName[]){
    struct_cliente_TCFS paquete;
    memset(&paquete,0,sizeof(paquete));
    char codigo[4];
    memcpy(codigo,"TCFS",4);

    memcpy(paquete.codop,codigo,4);
    paquete.codigo = 0;
    paquete.userIDSender = id;
    paquete.userIDReceiver = userIDReceiver;
    memcpy(paquete.fileName,fileName,sizeof(paquete.fileName));    

    enviarArchivo(fd, paquete);
    printf("[+]File header data sent successfully.\n");
}

void recibirArchivo(int fd, struct_cliente_TCFS paquete){
    int fileSize = paquete.fileSize;
    char buf[FILEBUFSIZE];
    int file = open(paquete.fileName,O_CREAT|O_RDWR,0666);
    miPerror("open",file);

    printf("creando archivo en disco...\n");

    int n = 0;
    long int totRead = 0;

    while( totRead < fileSize){

        long int delta = fileSize - totRead;
        memset(&buf,0,FILEBUFSIZE);
        if(FILEBUFSIZE <= delta){
            n = read(fd, buf, FILEBUFSIZE);
        }else{
            n = read(fd, buf, delta);
        }
        miPerror("read",n);

        if (n > 0){
            totRead+=n;
            int m = write(file,buf,n);
            miPerror("write",m);
        }
    }

    close(file);

    printf("archivo en disco creado\n");

}

void handleRecibirArchivo(int fd){
    struct_cliente_TCFS paquete;

    printf("recibiendo archivo\n");
    
    memset(&paquete,0,sizeof(paquete));
    
    char codigo[4];
    memcpy(codigo,"TCFS",4);

    int n = read(fd, &paquete, sizeof(paquete));
    miPerror("read",n);

    if(memcmp(codigo,paquete.codop,4) != 0){
        printf("El paquete recibido no es un header de archivo\n");
        return;
    }

    if(paquete.codigo != 0){
        printf("Error desconocido con el header de archivo\n");
        return;
    }

    recibirArchivo(fd,paquete);

    printf("[id:%d]%s\n",paquete.userIDSender,paquete.fileName);
    printf("archivo creado [%s]\n",paquete.fileName);
}

void preguntarPorUsuario(char userName[]){
    struct_cliente_UDPS paquete;
    struct_cliente_UDPS paqueteRecibido;
    memset(&paquete,0,sizeof(paquete));
    memset(&paqueteRecibido,0,sizeof(paqueteRecibido));

    memcpy(paquete.codop,"UDPS",4);
    memcpy(paquete.userName,userName,sizeof(paquete.userName));

    socklen_t servaddr_len;
    servaddr_len = sizeof(servaddr);
        
    int s = sendto(sockfdUDP, &paquete, sizeof(paquete), 0, (struct sockaddr *)&servaddr, servaddr_len);
    miPerror("sendto", s);

    printf("me bloquee antes del recvfrom(preguntar por usuario)\n");

    ssize_t n = recvfrom(sockfdUDP,&paqueteRecibido,sizeof(paqueteRecibido),0,NULL, NULL);
    miPerror("recvfrom", n);

    printf("me bloquee despues del recvfrom(preguntar por usuario)\n");

    if(paqueteRecibido.codigo == 0){
        printf("el usuario %s está conectado!\n",userName);
    }else if(paqueteRecibido.codigo == -1){
        printf("el usuario %s NO está conectado!\n",userName);
    }else if(paqueteRecibido.codigo == -2){
        printf("el usuario %s NO está registrado!\n",userName);
    }
}

void chatear(int fd, int idUser){
    int cerrar = 0;

    int opcion;

    while(!cerrar){

        printf("0:cerrar sesion\n");
        printf("1:enviar mensaje\n");
        printf("2:imprimir mensajes en el buzón\n");
        printf("3:recibir archivo\n");
        printf("4:enviar archivo\n");
        printf("5:preguntar por usuario\n");


        scanf("%d",&opcion);

        if(opcion == 0){
            printf("cerrando sesion...\n");
            cerrarSesion(fd);
            cerrar = 1;
        }else if(opcion == 1){
            char msj[MSJ_SIZE];
            memset(msj,0,sizeof(msj));

            printf("ingrese mensaje y presione enter:\n");
            vaciarStdin();
            fgets(msj, MSJ_SIZE-1, stdin);
            msj[MSJ_SIZE-1] = '\0';

            printf("mensaje a enviar:%s\n",msj);

            enviarMensaje(fd,msj,idUser);

        }else if(opcion == 2){
            printf("imprimiendo mensajes:\n");
            imprimirMensaje(fd);
        }else if(opcion == 3){
            //recibir archivo
            printf("esperando archivo...\n");
            handleRecibirArchivo(fd);
        }
        else if(opcion == 4){
            char filename[FILENAME_SIZE];
            //recibir archivo
            printf("Ingrese nombre del archivo y presione enter:\n");
            vaciarStdin();
            fgets(filename, FILENAME_SIZE-1, stdin);
            filename[strcspn(filename,"\n")] = '\0';

            printf("nombre del archivo a enviar:%s\n",filename);

            handleEnviarArchivo(fd, idUser, filename);
        }
        else if(opcion == 5){
            char userName[USERNAME_SIZE];

            memset(userName,0,sizeof(userName));

            printf("ingrese nombre de usuario y presione enter:\n");
            vaciarStdin();
            fgets(userName, USERNAME_SIZE-1, stdin);
            userName[strcspn(userName,"\n")] = '\0';

            printf("usuario por el cual preguntar: %s\n",userName);

            preguntarPorUsuario(userName);
        }
        else{
            printf("opcion no definida\n");
        }

    }
}


void iniciarChat(int fd){
    int idChat;

    printf("Ingrese el id del usuario con el que quiere chatear\n");
    scanf("%d",&idChat);

    chatear(fd,idChat);
}



int main()
{
    

    // socket create and verification
    sockfdUDP = socket(AF_INET, SOCK_DGRAM, 0);
    miPerror("socket",sockfdUDP);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("socket creation failed...\n");
        exit(0);
    }
    else if(sockfdUDP == -1){
        printf("socket UDP creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);


    // registrar 2 usuarios
    enviarRegister(sockfdUDP,servaddr);

    // connect the client socket to server socket
    if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0)
    {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
        printf("connected to the server..\n");


    printf("ingrese id para iniciar sesion\n");
    scanf("%d",&id);

    enviarLogin(sockfd);

    iniciarChat(sockfd);

    // close the socket
    close(sockfd);
    close(sockfdUDP);
}