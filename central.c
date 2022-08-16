

/*
file name: central.c
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


/*
**central.c
*/

//For TCP connection
#define TCP_PORT_TO_CLIENT_A "25913"   //central port number connected to clientA
#define TCP_PORT_TO_CLIENT_B "26913"   //central port number connected to clientB
#define LOCAL_HOST  "127.0.0.1"        //IP address for central

//For UDP connection
#define SERVER_T_UDP_PORT "21913"      //UDP port number to connect to serverT
#define SERVER_S_UDP_PORT "22913"      //UDP port number to connect to serverS
#define SERVER_P_UDP_PORT "23913"      //UDP port number to connect to serverP
#define CENTRAL_UDP_PORT  "24913"      //central port number for UDP connection

#define BACKLOG 10                   //max queing number
#define MAX_DATASIZE 1024            //max number of bytes to received at a time
#define MAX_BUFFERLEN 4000            //MAX datalen received from udp


/*
**Function definition
*/

// Get sockaddr, IPv4 or IPv6, reference from Beej:
void *get_in_addr(struct sockaddr *sa);
//TCP connection setup
int TCP_connection_configuration(char *portnum);
//UDP connection setup
int UDP_connection_configuration(char *portnum);
//UDP data request and send
int UDP_Request(int sockfd,char *query, char *recvdata, char *portnum);
//Reference from Beej
void sigchld_handler(int s);

//To split the received data with " "
void split_argvs(char *data,char *argv[])
{
    char *s = strtok(data, " ");
    int i =0;
    while(s != NULL)
    {
        argv[i++] = s;
        s = strtok (NULL," ");
    }
}

int main(int argc, char *argv[])
{  
    int tcp_sockfd_a,tcp_sockfd_b,
    udp_sockfd, new_sockfd1,new_sockfd2;
    struct sockaddr_storage clientA_addr, clientB_addr;
    socklen_t sin_size1, sin_size2;  //used for accept
    char s[INET6_ADDRSTRLEN],s1[INET6_ADDRSTRLEN];
    int bytesnum,bytesnum2;
    pid_t pida,pidb;

    char clientA_input[MAX_DATASIZE];
    char clientB_input[MAX_DATASIZE];
    char central_send_buf[MAX_DATASIZE];
    char udp_sendto[MAX_BUFFERLEN];
    char udp_recvfrom_T[MAX_BUFFERLEN];
    char udp_recvfrom_S[MAX_BUFFERLEN];
    char udp_recvfrom_P[MAX_BUFFERLEN];

    char *argv_buf[10],*argv_buf1[10];

    //Create tcp socket to clientA
    tcp_sockfd_a = TCP_connection_configuration(TCP_PORT_TO_CLIENT_A);
    //Create tcp socket to clientB
    tcp_sockfd_b = TCP_connection_configuration(TCP_PORT_TO_CLIENT_B);
    //Create udp socket
    udp_sockfd = UDP_connection_configuration(CENTRAL_UDP_PORT);


    sin_size1 = sizeof(clientA_addr);
    sin_size2 = sizeof(clientB_addr);
    printf("The Central server is up and running\n");
    //printf("Central server: waiting for connections...\n");
    while(1)
    { 
        //Connection from clientA
        new_sockfd1 = accept(tcp_sockfd_a,(struct sockaddr*)&clientA_addr,&sin_size1);
        if( new_sockfd1 == -1)
        {
            printf("Can't connect to clientA\n");
            continue;
        }
        inet_ntop(clientA_addr.ss_family,get_in_addr((struct sockaddr*)&clientA_addr),s,sizeof(s));

        if(!fork())
        {
            close(tcp_sockfd_a);
            if((bytesnum = recv(new_sockfd1,clientA_input,MAX_DATASIZE-1,0))==-1)
            {
                printf("Can't receive data from clientA\n");
                exit(1);
            }
            clientA_input[bytesnum] = '\0';
            printf("The Central server received input = '%s' from client using TCP over port 25913 \n",clientA_input);
        //}

        //Connection from clientB
        new_sockfd2 = accept(tcp_sockfd_b,(struct sockaddr*)&clientB_addr,&sin_size2);
        if( new_sockfd2 == -1)
        {
            printf("Can't connect to clientB\n");
            continue;
        }
        inet_ntop(clientB_addr.ss_family,get_in_addr((struct sockaddr*)&clientB_addr),s1,sizeof(s1));
        if(!fork())
        {
            close(tcp_sockfd_b);
            if((bytesnum2 = recv(new_sockfd2,clientB_input,MAX_DATASIZE-1,0))==-1)
            {
                printf("Can't receive data from clientB\n");
                exit(1);
            }
            clientB_input[bytesnum2] = '\0';
            printf("The Central server received input = '%s' from client using TCP over port 26913\n",clientB_input);
       
            fflush(stdout);
            //Connection through UDP
        sprintf(udp_sendto,"%s%s",clientA_input,clientB_input);

        char send_buf_2T[MAX_BUFFERLEN] = {"edgelist"};
        char send_buf_2S[MAX_BUFFERLEN] = {"scores"};
        char send_buf_2P[MAX_BUFFERLEN];
        UDP_Request(udp_sockfd,send_buf_2T,udp_recvfrom_T,SERVER_T_UDP_PORT);
        //printf("receive from serverT: %s\n",udp_recvfrom_T);
        UDP_Request(udp_sockfd,send_buf_2S,udp_recvfrom_S,SERVER_S_UDP_PORT);
        //printf("receive from serverS: %s\n",udp_recvfrom_S);

        strcat(udp_recvfrom_T,udp_recvfrom_S);
        strcat(udp_recvfrom_T,udp_sendto);
        strcpy(send_buf_2P,udp_recvfrom_T);    
        UDP_Request(udp_sockfd,send_buf_2P,udp_recvfrom_P,SERVER_P_UDP_PORT);

        //printf("recvfrom %s\n",udp_recvfrom_P);

        if(send(new_sockfd2,udp_recvfrom_P,strlen(udp_recvfrom_P),0) == -1)
            perror("send");
        printf("The Central server sent the results to client A\n"); 
        printf("The Central server sent the results to client B\n");
           
        
        }
        if(send(new_sockfd1,udp_recvfrom_P,strlen(udp_recvfrom_P),0) == -1)
            perror("send");

        

        
        close(new_sockfd1);
        close(new_sockfd2);
        exit(0);
        }
        
        
        
        
    }
    //close(tcp_sockfd_a);
    //close(tcp_sockfd_b);
    return 0;


}

/*
//To configurate TCP connection
*/
int TCP_connection_configuration(char *portnum)
{
    int sockfd, new_fd,rv; // listen on sockfd, new connection on new_fd
    struct addrinfo hints;
    struct addrinfo *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];

    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;    

    
    if ((rv = getaddrinfo(LOCAL_HOST, portnum, &hints, &servinfo)) != 0)  //use my local IP
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) 
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // release the space

    if (p == NULL) 
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) 
    {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) 
    {
        perror("sigaction");
        exit(1);
    }
 
    return sockfd;

}


/*
//To configurate UDP connection
*/
int UDP_connection_configuration(char *portnum)
{
    int socketfd,rv,bytesnum; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t addr_len;

 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    
    rv = getaddrinfo(LOCAL_HOST, portnum, &hints, &servinfo);
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
            perror("Central server: socket");
            continue;
        }
        if (bind(socketfd, p->ai_addr, p->ai_addrlen) == -1) 
        {
            close(socketfd);
            perror("Central server: bind");
            continue;
        }

        break;
    }

    if (p == NULL) 
    {
        fprintf(stderr, "Central server: failed to bind\n");
        exit(1);
    }

    freeaddrinfo(servinfo);   //release the spcace
   
    return socketfd;
}    

// To request and receive data through UDP connection according to the inputed port nunber
int UDP_Request(int sockfd,char *query, char *recvdata, char *portnum)
{
    int sendbytes,recvbytes,rv;
    struct addrinfo hints, *serverinfo, *p;
    char recv_data[MAX_BUFFERLEN];

    memset(&hints,0,sizeof(hints));
    if ((rv = getaddrinfo(LOCAL_HOST, portnum, &hints, &serverinfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and bind to the first we can
    for(p = serverinfo; p != NULL; p = p->ai_next) 
    {
        if ((socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) 
        {
            perror("Central client: socket");
            continue;
        }
        break;
    }
    if (p == NULL) 
    {
        fprintf(stderr, "Central client: failed to create udp socket\n");
        exit(1);
    }

    freeaddrinfo(serverinfo);   //release the space

    sendbytes = sendto(sockfd,query,strlen(query),0,p->ai_addr,p->ai_addrlen);
    if(sendbytes == -1)
    {
        perror("Central client: sendto");
        exit(1);
    }

    fflush(stdout);

    if(strcmp(portnum , SERVER_T_UDP_PORT) == 0)  
    {
        printf("The Central server sent a request to Backend-Server T\n");
        printf("The Central server received information from Backend-Server<T> using UDP over port %s\n",SERVER_T_UDP_PORT);
    }
    if(strcmp(portnum , SERVER_S_UDP_PORT) == 0) 
    {
        printf("The Central server sent a request to Backend-Server S\n");
        printf("The Central server received information from Backend-Server<S> using UDP over port %s\n",SERVER_S_UDP_PORT);
    }
    if(strcmp(portnum , SERVER_P_UDP_PORT) == 0)  
    {
        printf("The Central server sent a processing request to Backend-Server P\n");
        printf("The Central server received results from Backend-Server P\n");
    }
    //printf("listen: wating to recvform:...\n");


    if((recvbytes = recvfrom(sockfd,recv_data,sizeof(recv_data),0,NULL,NULL)) == -1)
    {
        perror("recvfrom");
        exit(1);
    }
    recv_data[recvbytes] = '\0';

    strcpy(recvdata,recv_data);

    //printf("recvdata: %s\n",recvdata);


    
   
}

void sigchld_handler(int s)
{
// waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) 
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);

    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



