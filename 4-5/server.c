#include <stdio.h>     
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>   
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>


// there is no queue in c(((
#include "../queue.h"


static volatile int keepRunning = 1;


#define MAXPENDING 10
#define CLIENT_WAIT 1
#define CLIENT_FINISH 2

typedef struct thread_args {
    int socket;
    void (*handler)(int);
} thread_args;

void intHandler(int dummy) {
    keepRunning = 0;
}


// Pointer to queue;
node_t *queue = NULL;


void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

void handleClient(int socket) {
    printf("[SYSTEM] Client connected!\n");
    int client_data[3];
    for(;;) {
        recv(socket, client_data, sizeof(client_data), 0);
        if (client_data[1] == -1) {
            break;
        }
        printf("[Server] New client in the queue #%d\n", client_data[0]);
        enqueue(&queue, client_data[0]);
    }
    close(socket);
    printf("[SYSTEM] Client disconected!\n");
}


void handleCutter(int socket) {
    printf("[SYSTEM] Cutter connected!\n");
    int client_data[3];
    client_data[0] = 0;
    for (;;) {
        recv(socket, client_data, sizeof(client_data), MSG_DONTWAIT);
        if (client_data[0] == -1) {
            close(socket);
            printf("[SYSTEM] Cutter disconected!\n");
            return;
        }
        if (queue == NULL) {
            sleep(1);
            continue;
        }
        client_data[0] = dequeue(&queue);
        send(socket, client_data, sizeof(client_data), 0);
        printf("[Server] Sent client #%d to cutter\n", client_data[0]);
        recv(socket, client_data, sizeof(client_data), 0);
        if (client_data[0] == -1) {
            printf("[SYSTEM] Cutter disconected!\n");
            return;
        }
        printf("[Server] Client #%d finished\n", client_data[0]);
        client_data[0] = 0;
    }
    client_data[0] = -1;
    send(socket, client_data, sizeof(client_data), 0);
    close(socket);
}


void *serviceThread(void *args) {
    int server_socket;
    int client_socket;
    int client_length;
    struct sockaddr_in client_addr;
    pthread_detach(pthread_self());
    server_socket = ((thread_args*)args)->socket;
    void (*handler)(int) = ((thread_args*)args)->handler;
    free(args);
    listen(server_socket, MAXPENDING);
    for (;;) {
        client_length = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_length);
        printf("[SYSTEM] New connection from %s\n", inet_ntoa(client_addr.sin_addr));
        handler(client_socket);
    }
}

void createServiceOnPort(char* name, void(*handler)(int), unsigned short server_port) {
    pthread_t serviceThreadId;
    int server_socket;
    int client_socket;
    struct sockaddr_in server_addr;

    if ((server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) DieWithError("socket() failed");
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;              
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_addr.sin_port = htons(server_port);

    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) DieWithError("bind() failed");

    printf("[SYSTEM] Service '%s' is running on %s:%d\n", name, inet_ntoa(server_addr.sin_addr), server_port);
    thread_args *args = (thread_args*) malloc(sizeof(thread_args));
    args->socket = server_socket;
    args->handler = handler;
    if (pthread_create(&serviceThreadId, NULL, serviceThread, (void*) args) != 0) DieWithError("pthread_create() failed");
}

int main(int argc, char *argv[])
{
    unsigned short client_port;
    unsigned short cutter_port;

    if (argc != 3)
    {
        fprintf(stderr, "Usage:  %s <Port for cutter> <Port for client>\n", argv[0]);
        exit(1);
    }

    cutter_port = atoi(argv[1]);
    client_port = atoi(argv[2]);

    createServiceOnPort("Cutter", handleCutter, cutter_port);
    createServiceOnPort("Clients", handleClient, client_port);

    for (;;) {
        sleep(1);
    }
    return 0;
}
