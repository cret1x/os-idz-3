#include <stdio.h>     
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>   
#include <string.h>
#include <unistd.h>
#include <pthread.h>


#define MAXPENDING 10
#define CLIENT_WAIT 1
#define CLIENT_FINISH 2

int current_client_id = -1;

typedef struct thread_args {
    int socket;
} thread_args;


void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

void handleClient(int client_socket) {
    printf("[Cutter] Got new client\n");
    int client_data[3];
    recv(client_socket, client_data, sizeof(client_data), 0);
    printf("[Cutter] Working on client #%d\n", client_data[0]);
    current_client_id = client_data[0];
    client_data[1] = CLIENT_WAIT;
    send(client_socket, client_data, sizeof(client_data), 0);

    sleep(2 + rand() % 2);

    client_data[1] = CLIENT_FINISH;
    send(client_socket, client_data, sizeof(client_data), 0);
    close(client_socket);
    printf("[Cutter] Finished client #%d\n", client_data[0]);
    current_client_id = -1;
}

void handleListener(int socket) {
    int data[1];
    printf("[SYSTEM] Listener connected\n");
    for(;;) {
        data[0] = current_client_id;
        send(socket, data, sizeof(int), 0);
        recv(socket, data, sizeof(int), 0);
        if (data[0] == -2) {
            break;
        }
        sleep(1);
    }
    printf("[SYSTEM] Listener disconnected\n");
    close(socket);
}

void *handleListenerThread(void *args) {
    int client_socket;
    pthread_detach(pthread_self());
    client_socket = ((thread_args*)args)->socket;
    free(args);
    handleListener(client_socket);
    return (NULL);
}


void *notifierThread(void *args) {
    pthread_t threadId;
    int server_socket;
    int client_socket;
    int client_length;
    struct sockaddr_in client_addr;
    pthread_detach(pthread_self());
    server_socket = ((thread_args*)args)->socket;
    free(args);
    listen(server_socket, MAXPENDING);
    for (;;) {
        client_length = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_length);
        printf("[SYSTEM] New connection from %s\n", inet_ntoa(client_addr.sin_addr));
        thread_args *args = (thread_args*) malloc(sizeof(thread_args));
        args->socket = client_socket;
        if (pthread_create(&threadId, NULL, handleListenerThread, (void*) args) != 0) DieWithError("pthread_create() failed");
    }
    return (NULL);
}


void *cutterThread(void *args) {
    int server_socket;
    int client_socket;
    int client_length;
    struct sockaddr_in client_addr;
    pthread_detach(pthread_self());
    server_socket = ((thread_args*)args)->socket;
    free(args);
    listen(server_socket, MAXPENDING);
    for (;;) {
        client_length = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_length);
        printf("[SYSTEM] New connection from %s\n", inet_ntoa(client_addr.sin_addr));
        handleClient(client_socket);
    }
    return (NULL);
}


void createServiceOnPort(char* name, void *(*func)(void*), unsigned short server_port) {
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
    if (pthread_create(&serviceThreadId, NULL, func, (void*) args) != 0) DieWithError("pthread_create() failed");
}

int main(int argc, char *argv[])
{
    unsigned short server_port;
    unsigned short notifier_port;

    if (argc != 3)
    {
        fprintf(stderr, "Usage:  %s <Server Port> <Notifier Port>\n", argv[0]);
        exit(1);
    }

    server_port = atoi(argv[1]);
    notifier_port = atoi(argv[2]);

    createServiceOnPort("Cutter", cutterThread, server_port);
    createServiceOnPort("Notifier", notifierThread, notifier_port);

    for (;;) {
        sleep(1);
    }
    return 0;
}
