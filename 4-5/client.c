#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

#define RCVBUFSIZE 3   /* Size of receive buffer */

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in serverAddr; /* Echo server address */
    unsigned short servPort;     /* Echo server port */
    char *servIP;                    /* Server IP address (dotted quad) */
    char *echoString;                /* String to send to echo server */
    int clientData[RCVBUFSIZE];     /* Buffer for echo string */
    int recvMsgSize, totalBytesRcvd;   /* Bytes read in single recv() 
                                        and total bytes read */

    if (argc < 3)    /* Test for correct number of arguments */
    {
       fprintf(stderr, "Usage: %s <Server IP> <Server Port>\n",
               argv[0]);
       exit(1);
    }

    servIP = argv[1];             /* First arg: server IP address (dotted quad) */
    servPort = atoi(argv[2]);         /* Second arg: string to echo */

    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&serverAddr, 0, sizeof(serverAddr));     /* Zero out structure */
    serverAddr.sin_family      = AF_INET;             /* Internet address family */
    serverAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
    serverAddr.sin_port        = htons(servPort);     /* Server port */

    /* Establish the connection to the echo server */
    printf("I am client #%d, want to got to cutter\n", getpid());
    if (connect(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
        DieWithError("connect() failed");

    clientData[0] = getpid();

    /* Send the data to the server */
    if (send(sock, clientData, RCVBUFSIZE, 0) != RCVBUFSIZE)
        DieWithError("send() sent a different number of bytes than expected");

    if ((recvMsgSize = recv(sock, clientData, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");

    printf("Getting hair cut\n");
    
     if ((recvMsgSize = recv(sock, clientData, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");

    printf("Finished\n");
    close(sock);
    exit(0);
}
