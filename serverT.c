
/*
file name: serverT.c
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

#define ServerT_PORT "21913"       //The port number used for serverT for udp connection
#define LOCAL_HOST "127.0.0.1"     //The local IP
#define MAX_DADAZIE 1024
#define MAXBUFLEN 4000              //MAX datalen for udp transimission
#define IFIN 20000
#define MAX_VERT 100
#define MAX_LEN 100
#define MAXQUEUE 10
#define FILEROUTE "/home/student/Desktop/source/edgelist.txt"   //file route




//Global variables

int get_textline();

void *get_in_addr(struct sockaddr *sa);

void get_network_graph(char *username, char *network_graph);

void split_argvs(char *argv[],char *data);

int get_index(char *argv[],int vertnum, char buf[MAX_LEN][MAX_LEN]);

void array_transfer(int vertnum, int number_buf[vertnum][vertnum], char letter_buf[100]);

int main()
{
    int socketfd,rv,bytesnum; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t addr_len = sizeof their_addr;
    char recv_buf[MAXBUFLEN],send_buf[MAXBUFLEN];
    char *argv[3];
 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    
    rv = getaddrinfo(LOCAL_HOST, ServerT_PORT, &hints, &servinfo);
    if (rv != 0) 
    {
        printf("getaddrinfo failed\n");
        return 1;
    }
    
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
        if ((socketfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) 
        {
            perror("ServerT: socket");
            continue;
        }
        if (bind(socketfd, p->ai_addr, p->ai_addrlen) == -1) 
        {
            close(socketfd);
            perror("ServerT: bind");
            continue;
        }
        break;
    }

    if (p == NULL) 
    {
        fprintf(stderr, "Central server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo);   //release the  space

    printf("The ServerT is up and running using UDP on port<%s>: \n",ServerT_PORT);

    fflush(stdout);
    


    while(1)
    {
        if((bytesnum = recvfrom(socketfd,recv_buf,MAXBUFLEN-1,0,
        (struct sockaddr*)&their_addr,&addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }
        recv_buf[bytesnum] = '\0';
    
        split_argvs(argv,recv_buf);

        //printf("1st input is: %s, 2nd input is: %s\n", argv[0],argv[1]);

        //get_index(argv,vernum,host_buf);

        printf("The ServerT received a request from Central to get the topology\n");

        //open the edgelist.txt file to get the relationship
        //realted to differnet hostname
        FILE *file = fopen(FILEROUTE, "r");
        if(file == NULL)
        {
            printf("File %s not found\n", FILEROUTE);
        }
        char str[100],str1[100];
        char *data_buf[100];
        char host_buf[MAX_LEN][MAX_LEN] = {"\0"};
    
        int linenum, hostnum = 0, edgenum, vernum;
        int temp;

        while(fgets(str,sizeof(str),file))
        {
            str[strlen(str) - 1] = '\0';
            split_argvs(data_buf,str);
            strcpy(host_buf[hostnum], data_buf[0]);
            strcpy(host_buf[++hostnum], data_buf[1]);
            hostnum++;      
        }
        //printf("hostname: %d\n",hostnum);
        // create matrix 
        //int matrix[hostnum][hostnum];
        int times = 0;
        int colum,line;
    
        for(int i=0;i<hostnum;++i)
        {
            for(int j =i+1;j < hostnum;++j)
            {
                if(strcmp(host_buf[i] ,host_buf[j]) == 0)
                {
                    for(int k = j;k< hostnum-1;k++)
                        strcpy(host_buf[k] , host_buf[k+1]);
                    j = i;
                    --hostnum;
                }
            }   
        }
        
        //printf("hostname: %d\n",hostnum);
        int matrix[hostnum][hostnum];

        for(int i = 0; i < hostnum; i++)
        {
            for(int j =0; j < hostnum; j++)
            {
                matrix[i][j] = 0;
            }
        }


        //printf("\n");

        memset(&data_buf,0,sizeof(data_buf));
/*
        for(int i =0;i<hostnum;i++)
        {
            printf("host %d is %s\n",i,host_buf[i]);
        }
*/
        //open file again to get the matrix
        FILE *file1 = fopen(FILEROUTE, "r");
        while(fgets(str,sizeof(str),file1))
        {
            str[strlen(str) - 1] = '\0';
            split_argvs(data_buf,str);
            //printf("GET IN data1 %s \n",*data_buf);
            //printf("GET IN data1 %s \n",*(data_buf+1));
            colum = get_index(data_buf,hostnum,host_buf);
            line = get_index((data_buf+1),hostnum,host_buf);
            if((colum <= hostnum)&&(line <= hostnum))
            {
                matrix[colum][line]= matrix[line][colum] = 1;
            } 
        }

    
        fclose(file);
        fclose(file1);
    
        //transfer the matrix to another format before sending
        memset(&send_buf,0,sizeof send_buf);
        array_transfer(hostnum,matrix,send_buf);
        //printf("send_buf : %s \n",send_buf);
        char temp_send[MAX_LEN];
        for(int i =0;i<hostnum;i++)
        {
            sprintf(temp_send, "%s/", host_buf[i]);
            strcat(send_buf,temp_send);
        }
        strcat(send_buf,"\n");
        //printf("send_buf : %s \n",send_buf);
        if((bytesnum = sendto(socketfd,send_buf,strlen(send_buf),0,
        (struct sockaddr*)&their_addr,addr_len)) == -1)
        {
            perror("ServerT: sendto");
            exit(1);
        }
        printf("The ServerT finished sending the topology to the Central\n");

    }
    return 0;
}

//To get the line number of a txt file
int get_textline(const char *filename)
{
    FILE *file = fopen(filename, "r");
    int count = 0;
    if(file != NULL)
    {
        while(!feof(file))
        {
            if(fgetc(file) == '\n')
                count++;
        }
    }
    fclose(file);
    return count;
}
     
void split_argvs(char *argv[],char *data)
{
    char *s = strtok(data, " ");
    int i =0;
    while(s != NULL)
    {
        argv[i++] = s;
        s = strtok (NULL," ");
    }

}
//Function to get the index according to hostname
int get_index(char *argv[],int vertnum, char buf[MAX_LEN][MAX_LEN])
{
    for(int i = 0; i < vertnum; i++)
    {
        if(strcmp(*argv,buf[i])==0)
        {
            return i;
        }
            
    }
}
//Functions to pack up the matrix before sending
void array_transfer(int vertnum, int number_buf[vertnum][vertnum], char letter_buf[MAXBUFLEN])
{
    int j,i;
    //printf("send_buf : %s \n",letter_buf);printf("The Central server sent the results to client A\n");
    for(i=0 ; i < vertnum; i++)
    {
        for(j=0 ; j < vertnum; j++)
        {
            if(number_buf[i][j] == 0)
                //letter_buf[(i*vertnum) + j] = "0";
                strcat(letter_buf,"0");
            if(number_buf[i][j] == 1)
                //letter_buf[(i*vertnum) + j] = "1";
                strcat(letter_buf,"1");
        }
        strcat(letter_buf," ");
        //printf("j = %d\n",j);
    }
    strcat(letter_buf,"\n");
    //printf("send_buf : %s \n",letter_buf);
}

