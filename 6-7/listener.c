#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <signal.h>

#define RCVBUFSIZE 3   /* Size of receive buffer */

static volatile int keepRunning = 1;

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}


void intHandler(int dummy) {
    keepRunning = 0;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, intHandler);
    int client_socket;
    struct sockaddr_in server_addr;
    unsigned short server_port;
    int recv_msg_size;
    char *server_ip;

    if (argc < 3)    /* Test for correct number of arguments */
    {
       fprintf(stderr, "Usage: %s <Server IP> <Server Port>\n", argv[0]);
       exit(1);
    }

    server_ip = argv[1];
    server_port = atoi(argv[2]);

    if ((client_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    memset(&server_addr, 0, sizeof(server_addr));     /* Zero out structure */
    server_addr.sin_family      = AF_INET;             /* Internet address family */
    server_addr.sin_addr.s_addr = inet_addr(server_ip);   /* Server IP address */
    server_addr.sin_port        = htons(server_port);     /* Server port */

    printf("Im Listener process\n");

    if (connect(client_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
        DieWithError("connect() failed");

    printf("Connected to server\n");
    int data[1];
    while(keepRunning) {
        if ((recv_msg_size = recv(client_socket, data, sizeof(int), 0)) < 0)
            DieWithError("recv() failed");
        printf("Current client id: %d\n", data[0]);
        if (keepRunning == 0) {
            data[0] = -2;
        }
        send(client_socket, data, sizeof(int), 0);
    }
    close(client_socket);
    exit(0);
}
