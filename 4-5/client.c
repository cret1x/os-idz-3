#include <stdio.h>   
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

int main(int argc, char *argv[])
{
    int client_socket;
    int client_count;
    struct sockaddr_in server_addr;
    unsigned short server_port;
    int recv_msg_size;
    char *server_ip;
    int client_data[3];

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
    client_data[1] = 0;
    // infinitly generae clients for server
    for (int i = 0;i < client_count; i++) {
        client_data[0] = i + 1;
        printf("Generating new client #%d\n", i + 1);
        send(client_socket, client_data, sizeof(client_data), 0);
        sleep(2);
    }
    client_data[1] = -1;
    send(client_socket, client_data, sizeof(client_data), 0);
    close(client_socket);
    exit(0);
}
