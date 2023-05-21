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
    printf("Sending stop signal\n");
    keepRunning = 0;
}

int current_client_id = -1;

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
    while(keepRunning) {
        recv(socket, client_data, sizeof(client_data), 0);
        if (client_data[0] == -1) {
            printf("[SYSTEM] Client disconected!\n");
            close(socket);
            return;
        }
        printf("[Server] New client in the queue #%d\n", client_data[1]);
        enqueue(&queue, client_data[1]);
        client_data[0] = 0;
        send(socket, client_data, sizeof(client_data), 0);
    }
    recv(socket, client_data, sizeof(client_data), 0);
    client_data[0] = -1;
    send(socket, client_data, sizeof(client_data), 0);
    close(socket);
}


void handleCutter(int socket) {
    printf("[SYSTEM] Cutter connected!\n");
    int client_data[3];
    client_data[0] = 0;
    while(keepRunning) {
        if (queue == NULL) {
            client_data[1] = -1;
        } else {
            printf("[Server] Sent client #%d to cutter\n", client_data[0]);
            client_data[1] = dequeue(&queue);
            current_client_id = client_data[1];
        }
        send(socket, client_data, sizeof(client_data), 0);
        recv(socket, client_data, sizeof(client_data), 0);
        if (client_data[0] == -1) {
            close(socket);
            printf("[SYSTEM] Cutter disconected!\n");
            return;
        }
        if (client_data[1] >= 0) {
            printf("[Server] Client #%d finished\n", client_data[1]);
        }
        sleep(1);
        current_client_id = -1;
        client_data[0] = 0;
    }
    client_data[0] = -1;
    send(socket, client_data, sizeof(client_data), 0);
    close(socket);
}

void* handleListener(void *args) {
    int socket;
    pthread_detach(pthread_self());
    socket = ((thread_args*)args)->socket;
    free(args);
    int data[3];
    data[0] = 0;
    printf("[SYSTEM] Listener connected\n");
    while(keepRunning) {
        data[1] = current_client_id;
        send(socket, data, sizeof(data), 0);
        recv(socket, data, sizeof(data), 0);
        if (data[0] == -1) {
            printf("[SYSTEM] Listener disconnected\n");
            close(socket);
            return NULL;
        }
        sleep(1);
    }
    data[0] = -1;
    send(socket, data, sizeof(data), 0);
    
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
    while(keepRunning) {
        client_length = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_length);
        printf("[SYSTEM] New connection from %s\n", inet_ntoa(client_addr.sin_addr));
        handler(client_socket);
    }
}

void *notifierThread(void *args) {
    int server_socket;
    int client_socket;
    int client_length;
    struct sockaddr_in client_addr;
    pthread_t threadId;
    pthread_detach(pthread_self());
    server_socket = ((thread_args*)args)->socket;
    free(args);
    listen(server_socket, MAXPENDING);
    while(keepRunning) {
        client_length = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_length);
        printf("[SYSTEM] New connection from %s\n", inet_ntoa(client_addr.sin_addr));
        thread_args *args = (thread_args*) malloc(sizeof(thread_args));
        args->socket = client_socket;
        if (pthread_create(&threadId, NULL, handleListener, (void*) args) != 0) DieWithError("pthread_create() failed");
    }
}
 
void createServiceOnPort(char* name, void(*handler)(int), unsigned short server_port, int isNot) {
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
    if (isNot) {
         if (pthread_create(&serviceThreadId, NULL, notifierThread, (void*) args) != 0) DieWithError("pthread_create() failed");
    } else {
        args->handler = handler;
        if (pthread_create(&serviceThreadId, NULL, serviceThread, (void*) args) != 0) DieWithError("pthread_create() failed");
    }
    
}

int main(int argc, char *argv[])
{
    signal(SIGINT, intHandler);
    unsigned short server_port;
    unsigned short client_port;
    unsigned short notifier_port;

    if (argc != 4)
    {
        fprintf(stderr, "Usage:  %s <Server Port> <Clients Port> <Notifier Port>\n", argv[0]);
        exit(1);
    }

    server_port = atoi(argv[1]);
    client_port = atoi(argv[2]);
    notifier_port = atoi(argv[3]);

    createServiceOnPort("Cutter", handleCutter, server_port, 0);
    createServiceOnPort("Clients", handleClient, client_port, 0);
    createServiceOnPort("Notifier", NULL, notifier_port, 1);
    while(keepRunning) {
        sleep(1);
    }
    sleep(2);
    return 0;
}
