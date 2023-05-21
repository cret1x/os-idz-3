#include <stdio.h>   
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

static volatile int keepRunning = 1;

void intHandler(int dummy) {
    keepRunning = 0;
}

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

int main(int argc, char *argv[])
{
    struct sigaction act;
    act.sa_handler = intHandler;
    act.sa_flags = SA_RESTART;
    sigaction(SIGINT, &act, NULL);

    int client_socket;
    struct sockaddr_in server_addr;
    unsigned short server_port;
    int recv_msg_size;
    char *server_ip;
    int dataPacket[3]; // [type, data, optional]

    if (argc != 3)
    {
       fprintf(stderr, "Usage: %s <Server IP> <Server Port>\n", argv[0]);
       exit(1);
    }

    server_ip = argv[1];
    server_port = atoi(argv[2]);

    if ((client_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) DieWithError("socket() failed");

    memset(&server_addr, 0, sizeof(server_addr));  
    server_addr.sin_family      = AF_INET;        
    server_addr.sin_addr.s_addr = inet_addr(server_ip); 
    server_addr.sin_port        = htons(server_port);

    if (connect(client_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) DieWithError("connect() failed");
    dataPacket[0] = 0;
    dataPacket[1] = 0;
    dataPacket[2] = 0;
    int client_id;
    printf("Waiting for new client\n");
    while(keepRunning) {
        recv(client_socket, dataPacket, sizeof(dataPacket), 0);
        if (dataPacket[0] == -1) {
            printf("Server shutdown\n");
            close(client_socket);
            return 0;
        }
        client_id = dataPacket[1];
        if (client_id == -1) {
            dataPacket[0] = 0;
            dataPacket[1] = -1;
            send(client_socket, dataPacket, sizeof(dataPacket), 0);
            continue;
        }
        printf("Got new client #%d\n", client_id);
        sleep(2 + rand() % 2);
        printf("Finished with client #%d\n", client_id);
        send(client_socket, dataPacket, sizeof(dataPacket), 0);
        dataPacket[0] = 0;
        sleep(1);
    }
    recv(client_socket, dataPacket, sizeof(dataPacket), 0);
    dataPacket[0] = -1;
    send(client_socket, dataPacket, sizeof(dataPacket), 0);
    close(client_socket);
    return 0;
}
