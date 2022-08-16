
/*
file name: serverS.c
coder: Junyu Huang
USC-ID: 8109163913
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>

#define ServerS_PORT "22913"            //The port number used for serverS for udp connection
#define LOCAL_HOST "127.0.0.1"          //The local IP
#define MAX_DADAZIE 1024

#define MAX_LEN 512                     //MAX datalen for udp transimission
#define SCORE_NOT_FOUND "The score is not found"
#define MAXLINE 6
#define FILEROUTE "/home/student/Desktop/source/scores.txt"     //file route

//Global variables

void *get_in_addr(struct sockaddr *sa);


void split_argvs(char *argv[],char *data)
{
    char *s = strtok(data, "  ");
    int i =0;
    while(s != NULL)
    {
        argv[i++] = s;
        s = strtok (NULL," ");
    }
}


int main()
{
    int socketfd,rv,bytesnum; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t addr_len = sizeof(their_addr);
    char recv_buf[MAX_LEN],send_buf[MAX_LEN];;
    char *argv[3];
 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    
    rv = getaddrinfo(LOCAL_HOST, ServerS_PORT, &hints, &servinfo);
    if (rv != 0) 
    {
        printf("getaddrinfo failed\n");
        return -1;
    }
    
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
        if ((socketfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) 
        {
            perror("ServerS: socket");
            continue;
        }
        if (bind(socketfd, p->ai_addr, p->ai_addrlen) == -1) 
        {
            close(socketfd);
            perror("ServerS: bind");
            continue;
        }
        break;
    }

    if (p == NULL) 
    {
        fprintf(stderr, "Central server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); 

    printf("The ServerS is up and running using UDP on port<%s>: \n",ServerS_PORT);

    
    
    while(1)
    {

        FILE *file = fopen(FILEROUTE, "r");
        if(file == NULL)
        {
            printf("File %s not found\n", FILEROUTE);
        }
        char str[100];
        char *data_buf[100];
        char host[100][30] = {"\0"};
        char temp_send[MAX_LEN];
    
        int hostnum = 0,temp;
        memset(&send_buf,0,sizeof(send_buf));
        memset(&temp_send,0,sizeof(temp_send));
        //open the scores.txt file to get the scores
        while(fgets(str,sizeof(str),file))
        {
            str[strlen(str) - 1] = '\0';
            split_argvs(data_buf,str);
            strcpy(host[hostnum], data_buf[0]);
            //printf("hostname is %s\n",host[hostnum]);
            strcpy(host[++hostnum], data_buf[1]);
            //printf("data is %s\n",host[hostnum]);
            sprintf(temp_send,"%s %s/",host[hostnum-1],host[hostnum]);
            strcat(send_buf,temp_send);
            hostnum++;      
        }
        strcat(send_buf,"\n");
        //printf("sendbuf is %s\n",send_buf);
        if((bytesnum = recvfrom(socketfd,recv_buf,MAX_LEN-1,0,
        (struct sockaddr*)&their_addr,&addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }
        recv_buf[bytesnum] = '\0';
    
        //split_argvs(argv,recv_buf);
        //printf("The buf: %s\n",recv_buf);
        printf("The ServerS received a request from Central to get the scores\n");

        //send the packup scores buffer to the central
        if((bytesnum = sendto(socketfd,send_buf,strlen(send_buf),0,
        (struct sockaddr*)&their_addr,addr_len)) == -1)
        {
            perror("ServerP: sendto");
            exit(1);
        }
        printf("The ServerS finished sending the scores to the Central\n");

    }
    return 0;
}
