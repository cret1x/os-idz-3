#include <stdio.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

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

    if (argc != 3)
    {
       fprintf(stderr, "Usage: %s <Server IP> <Server Port>\n", argv[0]);
       exit(1);
    }

    server_ip = argv[1];
    server_port = atoi(argv[2]);

    if ((client_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET; 
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port        = htons(server_port);

    printf("I am Listener process\n");

    if (connect(client_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
        DieWithError("connect() failed");

    printf("Connected to server\n");
    int data[1];
    while(keepRunning) {
        if ((recv_msg_size = recv(client_socket, data, sizeof(int), 0)) < 0) DieWithError("recv() failed");
        printf("Current client id: %d\n", data[0]);
        if (keepRunning == 0) {
            data[0] = -2;
        }
        send(client_socket, data, sizeof(int), 0);
    }
    close(client_socket);
    exit(0);
}
