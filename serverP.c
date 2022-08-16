
/*
file name: serverP.c
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
#include <float.h>
#include <math.h>
#include <stdbool.h>

#define ServerP_PORT "23913"            //The port number used for serverP for udp connection
#define CENTRAL_UDP_PORT  "24913"       //Central's port number
#define LOCAL_HOST "127.0.0.1"          //The local IP
#define MAX_DADAZIE 1024
#define MAXBUFLEN 4000                   //MAX datalen for udp transimission
#define IFIN 20000
#define MAXVERT 100
#define INPUTNOTFOUND 65535
#define MAX_DIST 1000                   //When two nodes are not connected

char udp_buf[MAXBUFLEN] = "\0";
int prev[50];

//Global variables

//Get the socket address
void *get_in_addr(struct sockaddr *sa);

//To analyze the received with " "
int split_argvs(char *data,char *argv[])
{
    char *s = strtok(data, " ");
    int i =0;
    while(s != NULL)
    {
        argv[i++] = s;
        s = strtok (NULL," ");
    }
    return i;
}
//To analyze the received with "\n"
void split_argvs1(char *data,char *argv[])
{
    char *s = strtok(data, "\n");
    int i =0;
    while(s != NULL)
    {
        argv[i++] = s;
        s = strtok (NULL,"\n");
    }
}
//To analyze the received with "/"
void split_argvs2(char *data,char *argv[])
{
    char *s = strtok(data, "/");
    int i =0;
    while(s != NULL)
    {
        argv[i++] = s;
        s = strtok (NULL,"/");
    }
}

int get_vernum(int bytenum)
{
    int new_bytenum = bytenum - 1;
    int j;
    for(int i = 0; i <= bytenum/2; i++)
    {
        j = i+1;
        if(new_bytenum == i*j)
            return i;
    }
}

//Dijsktra Algorithm to find the shorest path from src to dest
float  Dijkstra(int V,float graph[V][V],float dist[V],int src_idx,int dest_idx)
{
    int i,j;
    int flag[V];

    //Initialize all the parameters
    for(i= 0;i<V;i++)
    {      
        prev[i] = i;           //prev array to store teh shortest path tree
        dist[i] = MAX_DIST;    //the shortest distance from src to i
        flag[i] = 0;           //set to 1 if the V is included
    }

    dist[src_idx] = 0;
    for(i = 0;i<V;i++)
    {
        float min = MAX_DIST;
        int k = -1;
        for(j = 0;j<V;j++)
        {
            if(!flag[j] &&(dist[j] < min))        
            {
                min = dist[j];
                k = j;
            }   
        }

        if(k==-1)
            break;
        flag[k] = 1;
        
        for(j = 0;j<V;j++)
        {
            //update the shortest path according to next node
            //tmp = (graph[k][j]==MAX_DIST ? MAX_DIST:(min+graph[k][j]));
            if(flag[j]==0 &&graph[k][j]!=MAX_DIST && dist[k]+graph[k][j]<dist[j])
            //if(flag[j]==0&&(tmp<dist[j]))
            {
                //dist[j] = tmp;
                prev[j] = k;
                dist[j] = dist[k]+graph[k][j];
            }
        }
    }

    //return the shortest path
    return dist[dest_idx];
    
}

//Function used to get the path from src to dest
void resultpath(int s, int v, char p[100][100],int vertnum)
{
    if(v == s)
    {
        strcat(udp_buf,p[s]);
        strcat(udp_buf," ");
        return;
    }
    resultpath(s,prev[v],p,vertnum);
    strcat(udp_buf,p[v]);
    strcat(udp_buf," ");
}


int main()
{
    int socketfd,rv,bytesnum; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t addr_len;
    char recv_buf[MAXBUFLEN];
    char send_buf[MAXBUFLEN];
    char *argv[100],*argv1[100],*argv2[100],*argv3[100];
    char *temp_buf[100];
    char matrix_buf[100];
    char name_buf[100];
    char client_input_buf[100];
    char scores_buf[100];
    char host_buf[100][100],host[100][100];
    
    float shortest_dis = 0;
    char path_result[200] = "";
    
    int vertnum,inputA,inputB,src_index,dest_index;
    
 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    
    rv = getaddrinfo(LOCAL_HOST, ServerP_PORT, &hints, &servinfo);
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
            perror("ServerP: socket");
            continue;
        }
        if (bind(socketfd, p->ai_addr, p->ai_addrlen) == -1) 
        {
            close(socketfd);
            perror("ServerP: bind");
            continue;
        }
        break;
    }

    if (p == NULL) 
    {
        fprintf(stderr, "Central server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo);   //release the space

    printf("The ServerT is up and running using UDP on port<%s>: \n",ServerP_PORT);
    
    addr_len = sizeof(their_addr);
    while(1)
    {
        if((bytesnum = recvfrom(socketfd,recv_buf,MAXBUFLEN-1,0,
        (struct sockaddr*)&their_addr,&addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }
        recv_buf[bytesnum] = '\0';

        //printf("receive is :%s\n",recv_buf);
    
        /////// to get matrix /////
        split_argvs1(recv_buf,argv);

        strcpy(matrix_buf,argv[0]);
        /////// to get hostname /////
        strcpy(name_buf,argv[1]);
        /////// to get score /////
        strcpy(scores_buf,argv[2]);
        /////// to get input from clientA and B /////
        strcpy(client_input_buf,argv[3]);

        //printf("receive matrix :%s\n",matrix_buf);
        //printf("receive name :%s\n",name_buf);
        //printf("receive scores:%s\n",scores_buf);
        //printf("client input:%s\n",client_input_buf);

        vertnum =  split_argvs(matrix_buf,argv);
        float host_score[vertnum];
        float matrix[vertnum][vertnum];
        int host_index[vertnum];

        split_argvs2(name_buf,argv1);
        split_argvs2(scores_buf,argv2);
        split_argvs(client_input_buf,argv3);

        
        for(int i = 0; i<vertnum; i++)
        {
            strcpy(host[i],argv1[i]);
            //printf("host %d is %s \n",i,host[i]);
            host_index[i] = i;
        }
        
        for(int i = 0; i<vertnum; i++)
        {
            split_argvs(argv2[i],temp_buf);
            strcpy(host_buf[0],temp_buf[0]);
            strcpy(host_buf[1],temp_buf[1]);
            for(int j = 0;j<vertnum;j++)
            {
                if(strcmp(host[j],host_buf[0])==0)
                {
                    host_score[j] = atoi(host_buf[1]);
                    //printf("host %s score: %.2f\n",host[j],host_score[j]);
                }
            }
        }
        //initialize the matrix
        for(int i = 0; i<vertnum; i++)
        {
            for(int j = 0; j<vertnum; j++)
            {
                if(argv[i][j] == '0')
                    matrix[i][j] = 0;
                if(argv[i][j] == '1')
                    matrix[i][j] = 1;
            }
        }
        //update the matrix with maping gap
        for(int i = 0; i<vertnum; i++)
        {
            for(int j = 0; j<vertnum; j++)
            {
                if(matrix[i][j]!=0)
                {
                    //printf("score i = %.2f and score j = %.2f\n",host_score[i],host_score[j]);
                    matrix[i][j] = (abs(host_score[i]-host_score[j]))/(host_score[i]+host_score[j]);
                }
                //printf("%.2f ", matrix[i][j]);
            }  
            //printf("\n");       
        }
        
        for(int i = 0; i<vertnum; i++)
        {
            for(int j = 0; j<vertnum; j++)
            {
                if((i!=j)&&(matrix[i][j]==0))
                    matrix[i][j] =  MAX_DIST;
            }
        }
/*
        for(int i = 0; i<vertnum; i++)
        {
            for(int j = 0; j<vertnum; j++)
            {
                printf("%.2f  ",matrix[i][j]);
            }
            printf("\n");
        }
*/
        //get the src_idx and dest_idx
        int temp_i;
        //printf("argv0: %s: \n",argv3[0]);
        //printf("argv1: %s: \n",argv3[1]);
        for(int i = 0; i<vertnum; i++)
        {
            if(strcmp(host[i],argv3[0]) == 0) 
            {
                inputA = i;
                //printf("input from a is: %s (%d)\n",host[i],i);
            }
            if(strcmp(host[i],argv3[1]) == 0)
            {
                inputB = i;
                //printf("input from b is: %s (%d)\n",host[i],i);
            }  
        }
        src_index = inputA;
        dest_index = inputB;
        //printf("src_idx: %d  dest_idx: %d\n",src_index,dest_index);

        //use Dijkstra to find the shortest distance
        float dist[vertnum];
        memset(&udp_buf,0,sizeof udp_buf);
        //shortest_dis = Dijkstra(vertnum,matrix,src_index,dest_index,path_result,host_index);
        if(src_index == dest_index)
            shortest_dis = 0;
        if(src_index != dest_index)
            shortest_dis = Dijkstra(vertnum,matrix,dist,src_index,dest_index);


        //printf("shortest: %.2f\n",shortest_dis);
        
        //prepare the udp_buf that contain the
        //hostnames and maping gap 
        //according to different situation
        char send_tmp[100];
        if(shortest_dis!=MAX_DIST)
        {
            resultpath(src_index,dest_index,host,vertnum);
            char str[20];
            sprintf(str,"%.2f",shortest_dis);
            strcat(udp_buf,str);
           // printf("the path is %s\n",udp_buf);
        }
        if(shortest_dis==MAX_DIST)
        {
            strcpy(udp_buf,"NULL ");
            sprintf(send_tmp,"%s %s",argv3[0],argv3[1]);
            strcat(udp_buf,send_tmp);
            // printf("the path is %s\n",udp_buf);
        }

        printf("The ServerP received the topology and score infomation\n");

        if((bytesnum = sendto(socketfd,udp_buf,strlen(udp_buf),0,
        (struct sockaddr*)&their_addr,addr_len)) == -1)
        {
            perror("ServerP: sendto");
            exit(1);
        }

        printf("The ServerP finished sending the results to the Central\n");

    }
    return 0;
}



