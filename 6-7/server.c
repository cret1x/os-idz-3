#include <stdio.h>     
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>   
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>


#define MAXPENDING 5
#define RCVBUFSIZE 3


#define CLIENT_WAIT 1
#define CLIENT_FINISH 2

static volatile int keepRunning = 1;


typedef struct shared_memory {
    int current_client_id;
} shared_memory;

void intHandler(int dummy) {
    keepRunning = 0;
}

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

// cliendData: [client id, client status, _]

void HandleTCPClient(int client_socket, shared_memory* shmem) {
    printf("Got new client\n");
    int client_data[RCVBUFSIZE];      
    int recv_msg_size;               
    if ((recv_msg_size = recv(client_socket, client_data, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");
    printf("Working on client #%d\n", client_data[0]);
    shmem->current_client_id = client_data[0];
    client_data[1] = CLIENT_WAIT;
    if (send(client_socket, client_data, recv_msg_size, 0) != recv_msg_size)
            DieWithError("send() failed");
    sleep(2 + rand() % 2);
    client_data[1] = CLIENT_FINISH;
    if (send(client_socket, client_data, recv_msg_size, 0) != recv_msg_size)
            DieWithError("send() failed");
    close(client_socket);
    printf("Finished client #%d\n", client_data[0]);
    shmem->current_client_id = -1;
}

void handleListener(int socket, shared_memory* shmem) {
    int data[1];
    printf("Listener connected\n");
    for(;;) {
        data[0] = shmem->current_client_id;
        send(socket, data, sizeof(int), 0);
        recv(socket, data, sizeof(int), 0);
        if (data[0] == -2) {
            break;
        }
        sleep(1);
    }
    printf("Listener disconnected\n");
    close(socket);
}

int init_shmem(char memn[]) {
    int shm;
    int mem_size = sizeof(shared_memory);
    if ((shm = shm_open(memn, O_CREAT | O_RDWR, 0666)) == -1) {
      printf("Object is already open\n");
      perror("shm_open");
      return 1;
    } else {
          printf("Object is open: name = %s, id = 0x%x\n", memn, shm);
    }
    if (ftruncate(shm, mem_size) == -1) {
        printf("Memory sizing error\n");
        perror("ftruncate");
        return 1;
    } else {
        printf("Memory size set and = %d\n", mem_size);
    }
    return shm;
}

int main(int argc, char *argv[])
{
    int mem_size = sizeof(shared_memory);
    char memn[] = "shared-memory";
    //signal(SIGINT, intHandler);
    int server_socket;
    int client_socket;
    int listener_server_socket;
    int listener_client_socket;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    struct sockaddr_in server_listener_addr;
    struct sockaddr_in client_listener_addr;
    unsigned short server_port;
    unsigned short listener_port;
    unsigned int client_length;
    unsigned int listener_length;


    if (argc != 3)
    {
        fprintf(stderr, "Usage:  %s <Server Port> <Listener Port>\n", argv[0]);
        exit(1);
    }

    // init shmem;
    int shm = init_shmem(memn);
    void* addr = mmap(0, mem_size, PROT_WRITE, MAP_SHARED, shm, 0);
    if (addr == (int * ) - 1) {
        printf("Error getting pointer to shared memory\n");
        return 1;
    }
    shared_memory* shmem = addr;
    shmem->current_client_id = -1;
    server_port = atoi(argv[1]);
    listener_port = atoi(argv[2]);

    if ((server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    if ((listener_server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");


    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;              
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_addr.sin_port = htons(server_port);

    memset(&server_listener_addr, 0, sizeof(server_listener_addr));
    server_listener_addr.sin_family = AF_INET;              
    server_listener_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_listener_addr.sin_port = htons(listener_port);      

    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
        DieWithError("bind() failed");

    if (bind(listener_server_socket, (struct sockaddr *) &server_listener_addr, sizeof(server_listener_addr)) < 0)
        DieWithError("bind() failed");

    printf("Server IP address = %s:%d\n", inet_ntoa(client_addr.sin_addr), server_port);
    printf("Listener IP address = %s:%d\n", inet_ntoa(server_listener_addr.sin_addr), listener_port);

    int child = fork();
    if (child > 0) {
        // main server;
        if (listen(server_socket, MAXPENDING) < 0)
            DieWithError("listen() failed");
        for(;;)
        {
            client_length = sizeof(client_addr);
            if ((client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_length)) < 0)
                DieWithError("accept() failed");
            printf("Handling client %s\n", inet_ntoa(client_addr.sin_addr));
            HandleTCPClient(client_socket, shmem);
        }
    } else {
        
        // listener server;
        if (listen(listener_server_socket, MAXPENDING) < 0)
            DieWithError("listen() failed");
        for(;;)
        {
           
            listener_length = sizeof(client_listener_addr);
            if ((listener_client_socket = accept(listener_server_socket, (struct sockaddr *) &client_listener_addr, &listener_length)) < 0)
                DieWithError("accept() failed");
            printf("Handling listener %s\n", inet_ntoa(client_listener_addr.sin_addr));
            handleListener(listener_client_socket, shmem);
        }
    }
    close(shm);
    if(shm_unlink(memn) == -1) {
        printf("Shared memory is absent\n");
        perror("shm_unlink");
        return 1;
    }
    return 0;
}

