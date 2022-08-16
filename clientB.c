
/*
file name: clientB.c
coder: Junyu Huang
USC-ID: 8109163913
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

#define CENTRAL_PORT_FOR_B "26913" // the port client will be connecting to
#define LOCAL_HOST  "127.0.0.1"    //IP address of the central
#define MAXDATASIZE 100 // max number of bytes we can get at once

//Get the socket address
void *get_in_addr(struct sockaddr *sa)
{
    if(sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);  //IPV4
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);    //IPV6
}

//To analyze the received results from central
int analyze_result(char *argv[], char *data)
{
    char *p = strtok(data," ");
    int i = 0;
    while(p != NULL)
    {
        argv[i++] = p;
        p = strtok(NULL, " ");
    }
    return i;
}

int main(int argc, char *argv[])
{
    int sockfd, sendbytes,recvbytes,rv;
    char recv_buf[MAXDATASIZE],send_buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    char s[INET6_ADDRSTRLEN];
    //make sure the input is a hostname
    if(argc != 2)
    {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    //To get the address info
    if ((rv = getaddrinfo(LOCAL_HOST, CENTRAL_PORT_FOR_B, &hints, &servinfo)) != 0) 
    {
        printf("getaddrinfo failed\n");
        return 1;
    }

// loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) 
        {
            perror("clientB: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
        {
            close(sockfd);
            perror("clientB: connect");
            continue;
        }
        break;
    }
    if (p == NULL) 
    {
        fprintf(stderr, "clientB: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
    //clientB bootup finished
    printf("The client is up and running\n");
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure
  
    sprintf(send_buf,"%s ", argv[1]);
    sendbytes = send(sockfd, send_buf, strlen(send_buf), 0);
    if(sendbytes == -1)
    {
        perror("send");
        exit(1);
    }
    printf("The client sent %sto the Central Server\n",send_buf);
    
    //clientB received the shotrest path and the maping gap from central
    recvbytes = recv(sockfd,recv_buf,MAXDATASIZE-1,0);
    if(recvbytes == -1)
    {
        perror("recv");
        exit(1);
    }
    recv_buf[recvbytes] = '\0';

    //printf("recvfrom :%s\n",recv_buf);

    //Analyze the received data and print them
    int spacenum = analyze_result(argv,recv_buf);

    if(strcmp(argv[0],"NULL")==0)
    {
        printf("Found no compatibility for %s and %s\n",argv[1],argv[2]);
    }

    if(strcmp(argv[0],"NULL")!=0)
    {
        char *leftover;
        float gap = strtod(argv[spacenum-1],NULL);
        printf("Found compatibility for %s and %s\n",argv[spacenum-2],argv[0]);
        for(int i =spacenum-2;i>=0;i--)
        {
            if(i ==0)
            {
                printf("<%s>",argv[i]);
            }
            else
                printf("<%s>---",argv[i]);
        }
        printf("\n");

        printf("Compability score: %.2f\n",gap);
    }
  
    close(sockfd);
    
    return 0;

}
