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
    printf("Sending stop signal\n");
    keepRunning = 0;
}


int main(int argc, char *argv[])
{
    signal(SIGINT, intHandler);
    int client_socket;
    int client_count;
    struct sockaddr_in server_addr;
    unsigned short server_port;
    int recv_msg_size;
    char *server_ip;
    int dataPacket[3]; // [turn off signal, data, data]

    if (argc != 4)
    {
       fprintf(stderr, "Usage: %s <Server IP> <Server Port> <number of clients>\n", argv[0]);
       exit(1);
    }

    server_ip = argv[1];
    server_port = atoi(argv[2]);
    client_count = atoi(argv[3]);

    if ((client_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) DieWithError("socket() failed");

    memset(&server_addr, 0, sizeof(server_addr));  
    server_addr.sin_family      = AF_INET;        
    server_addr.sin_addr.s_addr = inet_addr(server_ip); 
    server_addr.sin_port        = htons(server_port);
    if (connect(client_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) DieWithError("connect() failed");
    


    dataPacket[0] = 0; // Run
    int current_client_id = 0;

    while (keepRunning && current_client_id < client_count) {
        printf("Generating new client #%d\n", current_client_id + 1);
        dataPacket[1] = current_client_id;
        dataPacket[0] = 0;
        send(client_socket, dataPacket, sizeof(dataPacket), 0);
        recv(client_socket, dataPacket, sizeof(dataPacket), 0);
        int status = dataPacket[0];
        if (status == -1) {
            printf("Got shutdown signal from server\n");
            close(client_socket);
            return 0;
        }
        current_client_id++;
        sleep(1 + rand() % 4);
    }
    dataPacket[0] = -1; // Stop
    send(client_socket, dataPacket, sizeof(dataPacket), 0);
    close(client_socket);
    return 0;
}
