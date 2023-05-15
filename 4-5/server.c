#include <stdio.h>     
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>   
#include <string.h>
#include <unistd.h>

#define MAXPENDING 5
#define RCVBUFSIZE 3


#define CLIENT_WAIT 1
#define CLIENT_FINISH 2

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

// cliendData: [client id, client status, _]

void HandleTCPClient(int client_socket) {
    printf("Got new client\n");
    int client_data[RCVBUFSIZE];      
    int recv_msg_size;               
    if ((recv_msg_size = recv(client_socket, client_data, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");
    printf("Working on client #%d\n", client_data[0]);
    client_data[1] = CLIENT_WAIT;
    if (send(client_socket, client_data, recv_msg_size, 0) != recv_msg_size)
            DieWithError("send() failed");
    sleep(2 + rand() % 2);
    client_data[1] = CLIENT_FINISH;
    if (send(client_socket, client_data, recv_msg_size, 0) != recv_msg_size)
            DieWithError("send() failed");
    close(client_socket);
    printf("Finished client #%d\n", client_data[0]);
}

int main(int argc, char *argv[])
{
    int server_socket;
    int client_socket;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    unsigned short server_port;
    unsigned int client_length;


    if (argc != 2)
    {
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        exit(1);
    }

    server_port = atoi(argv[1]);

    if ((server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;              
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_addr.sin_port = htons(server_port);      

    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
        DieWithError("bind() failed");

    printf("Server IP address = %s:%d Waiting for clients\n", inet_ntoa(client_addr.sin_addr), server_port);

    if (listen(server_socket, MAXPENDING) < 0)
        DieWithError("listen() failed");

    for (;;)
    {
        client_length = sizeof(client_addr);
        if ((client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_length)) < 0)
            DieWithError("accept() failed");
        printf("Handling client %s\n", inet_ntoa(client_addr.sin_addr));
        HandleTCPClient(client_socket);
    }
}

