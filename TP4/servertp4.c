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

// Dirección por defecto del servidor.
#define PORT 9898
#define IP "127.0.0.1"

typedef struct
{
    char nombre[26];
    int id;
    int conectado;
    struct sockaddr_in addr;
} Usuario;

typedef struct
{
    char nombre[26];
    int id;
} UsuarioEstructura;

static Usuario *users;
static int registeredUsers = 0;

// Tamaño del buffer en donde se reciben los mensajes.
#define BUFSIZE 100

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
    }
}

void registrarUsuario(char usuario[])
{
    registeredUsers++;
    Usuario nuevoUsuario;

    nuevoUsuario.conectado = 0;
    nuevoUsuario.id = registeredUsers;
    strncpy(nuevoUsuario.nombre, usuario, 26);

    users[registeredUsers] = nuevoUsuario;

    printf("usuario registrado %s", users[registeredUsers].nombre);
}

void loguearUsuario(char nombre[], int id, struct sockaddr_in addr)
{
    if (id <= 0 || id > registeredUsers)
    {
        printf("El id de usuario no existe");
        return;
    }

    if (!strcmp(users[id].nombre, nombre))
    {
        printf("Nombre de usuario incorrecto: %s", nombre);
        return;
    }

    if (users[id].conectado)
    {
        printf("El usuario ya está conectado");
        return;
    }

    users[id].conectado = 1;
    users[id].addr = addr;

    printf("Usuario [ID:%d] [Nombre:%s] conectado", id, nombre);
}

void handleLoguearUsuario(int socket, struct sockaddr_in src_addr)
{
    UsuarioEstructura estructura;
    int n = read(socket, &estructura, sizeof(estructura));
    miPerror("read", n);

    loguearUsuario(estructura.nombre, estructura.id, src_addr);
}
void handleUDP(int socket_udp, char buf[], struct sockaddr_in src_addr, socklen_t src_addr_len)
{
    // Recibe un mensaje entrante.
    ssize_t n = recvfrom(socket_udp, buf, BUFSIZE, 0, (struct sockaddr *)&src_addr, &src_addr_len);
    miPerror("recv", n);

    char *msj[2] = {
        "operacion no soportada\n",
        "registrarse\n",
    };

    int indicadorMSJ = 0;
    switch (buf[0])
    {
    case 'U':
        switch (buf[1])
        {
        case 'R':
            indicadorMSJ = 1;

            registrarUsuario(&buf[2]);
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }

    // Elimina '\n' al final del buffer.
    buf[n - 1] = '\0';

    // Imprime dirección del emisor y mensaje recibido.
    printf("[%s:%d][UDP] %s\n", inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port), buf);

    // Envía mensaje correspondiente
    buf[n - 1] = '\n';
    n = sendto(socket_udp, msj[indicadorMSJ], strlen(msj[indicadorMSJ]), 0, (struct sockaddr *)&src_addr, src_addr_len);
    miPerror("sendto", n);
}

void handleTCP(int sock, struct sockaddr_in src_addr)
{
    char buf[BUFSIZE];

    // en base a estos dos chars vemos que tamaño de mensaje leer
    char ch1[1];
    char ch2[1];

    char *msj[20] = {
        "operacion no soportada\n",
        "iniciar sesion\n",
        "iniciar chat\n",
        "enviar mensaje\n",
        "terminar sesion de chat\n",
        "enviar archivo\n",
        "recibir archivo\n",
        "unirse a grupo\n",
        "chatear en grupo\n",
        "salir de grupo\n",
        "e\n",
        "f\n",
        "g\n",
        "h\n",
        "i\n",
        "j\n",
    };

    int n;
    while (1)
    {
        memset(&buf, 0, BUFSIZE);
        memset(&ch1, 0, sizeof(ch1));
        memset(&ch2, 0, sizeof(ch2));

        int indicadorMSJ = 0;

        // leemos los codigos de operacion
        n = read(sock, ch1, 1);
        miPerror("read", n);
        n = read(sock, ch2, 1);
        miPerror("read", n);

        switch (ch1[0])
        {
        case 'U':
            switch (ch2[0])
            {
            case 'L':
                indicadorMSJ = 1;

                handleLoguearUsuario(sock, src_addr);
                break;
            case 'C':
                indicadorMSJ = 2;
                break;
            case 'M':
                indicadorMSJ = 3;
                break;
            case 'E':
                indicadorMSJ = 4;
                break;
            default:
                break;
            }
            break;
        case 'F':
            switch (ch2[0])
            {
            case 'S':
                indicadorMSJ = 5;
                break;
            case 'R':
                indicadorMSJ = 6;
                break;
            default:
                break;
            }
            break;

        case 'G':
            switch (ch2[0])
            {
            case 'J':
                indicadorMSJ = 7;
                break;
            case 'M':
                indicadorMSJ = 8;
                break;
            case 'E':
                indicadorMSJ = 9;
                break;

            default:
                break;
            }
            break;
        default:
            break;
        }

        // Elimina '\n' al final del buffer.
        buf[n - 1] = '\0';
        printf("[%s:%d][TCP] %s\n", inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port), buf);
        printf("%s", msj[indicadorMSJ]);
        // Envía eco
        buf[n - 1] = '\n';
        write(sock, msj[indicadorMSJ], strlen(msj[indicadorMSJ]));
    }
}

int main(int argc, char *argv[])
{
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
    if (argc == 3)
    {
        addr.sin_port = htons((uint16_t)atoi(argv[2]));
        inet_aton(argv[1], &(addr.sin_addr));
    }
    else
    {
        addr.sin_port = htons(PORT);
        inet_aton(IP, &(addr.sin_addr));
    }

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

    char buf[BUFSIZE];
    struct sockaddr_in src_addr;
    socklen_t src_addr_len;
    fd_set read_fds;

    FD_ZERO(&read_fds);

    for (;;)
    {
        // Agrega los sockets UDP y TCP en el set de descriptores de archivos
        // que se monitorearan por medio de select().
        FD_SET(socket_udp, &read_fds);
        FD_SET(socket_tcp, &read_fds);

        // Espera hasta que exista un paquete o una conexión en los sockets.
        int s = select(socket_tcp + 1, &read_fds, NULL, NULL, NULL);
        miPerror("select", s);

        memset(&src_addr, 0, sizeof(struct sockaddr_in));
        src_addr_len = sizeof(struct sockaddr_in);
        int sock;

        // Existe una conexión entrante en el socket TCP.
        if (FD_ISSET(socket_tcp, &read_fds))
        {
            int n;
            sock = accept(socket_tcp, (struct sockaddr *)&src_addr, &src_addr_len);
            miPerror("accept", sock);

            pid_t pid = fork();
            switch (pid)
            {
            case -1:
                miPerror("fork", -1);
            case 0:
                handleTCP(sock, src_addr);
                close(sock);
                exit(EXIT_SUCCESS);
            default:
                break;
            }
        }

        // Existe un paquete entrante en el socket UDP.
        if (FD_ISSET(socket_udp, &read_fds))
        {
            handleUDP(socket_udp, buf, src_addr, src_addr_len);
        }
    }

    // Cierra los sockets.
    close(socket_udp);
    close(socket_tcp);

    exit(EXIT_SUCCESS);
}
