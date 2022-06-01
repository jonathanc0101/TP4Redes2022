/**
 * Servidor echo
 * -------------
 *
 * El servidor escucha en el IP:PUERTO indicado como parámetro en la línea de
 * comando, o 127.0.0.1:8888 en caso de que no se indique una dirección.
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
 * Fecha: 2022-05-31
 * Última modificacion: 2022-05-31
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

// Dirección por defecto del servidor.
#define PORT 8888
#define IP "127.0.0.1"

// Tamaño del buffer en donde se reciben los mensajes.
#define BUFSIZE 100

// Descriptor de archivo del socket.
static int fd;
static int source_fd;

void miPerror(char nombre[], int valor)
{
    if (valor < 0)
    {
        perror(nombre);
    }
}

void handleConnection(int source_fd, struct sockaddr_in src_addr, int num_hijo)
{
    char buf[BUFSIZE];

    for (;;)
    {
        memset(&buf, 0, sizeof(buf));

        // Recibe un mensaje entrante.
        miPerror("read", read(source_fd, buf, sizeof(buf)));

        // Imprime dirección del emisor y mensaje recibido.
        printf("[Hijo:%d] [%s:%d] %s\n", num_hijo, inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port), buf);
    }

    close(source_fd);
    exit(EXIT_SUCCESS);
}

// Cierra el socket al recibir una señal SIGTERM.
void handler(int signal)
{
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    struct sockaddr_in addr;

    // Configura el manejador de señal SIGTERM.
    signal(SIGTERM, handler);

    // Crea el socket.
    fd = socket(AF_INET, SOCK_STREAM, 0);
    miPerror("socket", fd);

    // Estructura con la dirección donde escuchará el servidor.
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET; // ipv4
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

    int sockopt_res = setsockopt(fd, SOL_SOCKET, optname, &optval, sizeof(optval));
    miPerror("setsockopt", sockopt_res);

    // Asocia el socket con la dirección indicada. Tradicionalmente esta
    // operación se conoce como "asignar un nombre al socket".
    int b_res = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    miPerror("bind", b_res);

    int l_res = listen(fd, 2);
    miPerror("listen", l_res);

    printf("Escuchando en %s:%d ...\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

    struct sockaddr_in src_addr;
    socklen_t src_addr_len;

    int hijos_count = 0;
    for (;;)
    {
        // nos bloqueamos hasta que llegue una conexion
        source_fd = accept(fd, (struct sockaddr *)&src_addr, &src_addr_len);
        miPerror("accept", source_fd);

        if (fork() == 0)
        { // hijo
            handleConnection(source_fd, src_addr, hijos_count);
        }
        else
        {
            hijos_count++;
        }
    }

    // Cierra el socket.
    close(fd);

    exit(EXIT_SUCCESS);
}
