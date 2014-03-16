#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <sstream>
using namespace std;

#define ACK                   	2
#define NACK                  	3
#define MAXSIZE 				512
#define MAXLINE 				25600
#define SERVER_CONTROL_PORT 	6021
#define SERVER_DATA_PORT 		6020
#define CLIENT_CONTROL_PORT 	6081
#define CLIENT_DATA_PORT 		6080
#define CLIENT_HOST_ADDR		"127.0.0.1"

int readn(int sd,char *ptr,int size)
{         int no_left,no_read;
          no_left = size;
          while (no_left > 0) 
                     { no_read = read(sd,ptr,no_left);
                       if(no_read <0)  return(no_read);
                       if (no_read == 0) break;
                       no_left -= no_read;
                       ptr += no_read;
                     }
          return(size - no_left);
}

int writen(int sd,char *ptr,int size)
{         int no_left,no_written;
          no_left = size;
          while (no_left > 0) 
                     { no_written = write(sd,ptr,no_left);
                       if(no_written <=0)  return(no_written);
                       no_left -= no_written;
                       ptr += no_written;
                     }
          return(size - no_left);
}


string exec(string cmd) 
{
    FILE* pipe = popen(&cmd[0], "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    string result = "";
    while(!feof(pipe)) {
    	if(fgets(buffer, 128, pipe) != NULL)
    		result += buffer;
    }
    pclose(pipe);
    return result;
}

void SendString(int sd, string pkt)
{
	uint32_t msg_length = pkt.size();
	uint32_t sndMsgLength = htonl(msg_length); // Ensure network byte order
	send(sd,&sndMsgLength ,sizeof(uint32_t) ,MSG_CONFIRM); // Send the message length
	send(sd,pkt.c_str() ,msg_length ,MSG_CONFIRM); // Send the message data 
}

string RecvString(int csd)
{
	uint32_t msgLength;
	recv(csd,&msgLength,sizeof(uint32_t),0); // Receive the message length
	msgLength = ntohl(msgLength); // Ensure host system byte order

	vector<uint8_t> pkt; // Allocate a receive buffer
	pkt.resize(msgLength,0x00);

	recv(csd,&(pkt[0]),msgLength,0); // Receive the message data
	string tmp(pkt.begin(),pkt.end());

	return tmp;
}

void CreateNewSocket(unsigned int& sockid)
{
	if ((sockid = socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		printf("server: socket error : %d\n", errno); exit(0); 
	}

	#ifdef _PLATFORM_SOLARIS
			char yes='1';
		#else
			int yes=1;
		#endif

	if(setsockopt(sockid, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
	{
		perror("service_create(): socket opts");
	}
}

void BindSocketToLocalPort(unsigned int& sockid, int PORT)
{
	struct sockaddr_in my_addr;
	bzero((char *) &my_addr,sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(PORT);
	my_addr.sin_addr.s_addr = htons(INADDR_ANY);
	if (bind(sockid ,(struct sockaddr *) &my_addr,sizeof(my_addr)) < 0)
	{
		printf("server: bind  error :%d\n", errno); exit(0); 
	}
}


void ConnectToRemote(unsigned int& sockid, char* SERVER_HOST_ADDR, int SERVER_PORT)
{
	cout << "Connected to " << SERVER_HOST_ADDR << ":" << SERVER_PORT << endl;
	struct sockaddr_in server_addr; 
	bzero((char *) &server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_HOST_ADDR);
	server_addr.sin_port = htons(SERVER_PORT);
	if(connect(sockid ,(struct sockaddr *) &server_addr,sizeof(server_addr)) < 0){printf("client: connect  error :%d\n", errno); exit(0);}
}


void ReceiveFile(unsigned int& sockid, string filename)
{
	int i,ack,getfile,c,len, no_writen, num_blks,num_last_blk;
	FILE *fp;
	char in_buf[MAXSIZE];

    if ((fp = fopen(&filename[0],"w")) == NULL)
    {
    	printf(" client: local open file error \n");
    	exit(0);
    }

	if((readn(sockid,(char *)&num_blks,sizeof(num_blks))) < 0)
	{
		printf("client: read error on nblocks :%d\n",errno);
		exit(0);
	}

	num_blks = ntohs(num_blks);
	ack = ACK;  
	ack = htons(ack);
	if((writen(sockid,(char *)&ack,sizeof(ack))) < 0)
	{
		printf("client: ack write error :%d\n",errno);
		exit(0);
	}
   
	if((readn(sockid,(char *)&num_last_blk,sizeof(num_last_blk))) < 0)
	{
		printf("client: read error :%d on nbytes\n",errno);
		exit(0);
	}
	num_last_blk = ntohs(num_last_blk);  
	if((writen(sockid,(char *)&ack,sizeof(ack))) < 0)
	{
		printf("client: ack write error :%d\n",errno);
		exit(0);
	}

    for(i= 0; i < num_blks; i ++) 
    {
		if((readn(sockid,in_buf,MAXSIZE)) < 0)
		{
			printf("client: block error read: %d\n",errno);exit(0);
		}

		no_writen = fwrite(in_buf,sizeof(char),MAXSIZE,fp);
		if (no_writen == 0)
		{
			printf("client: file write error\n");exit(0);
		}
		if (no_writen != MAXSIZE) 
		{
			printf("client: file write  error : no_writen is less\n");exit(0);
		}

		if((writen(sockid,(char *)&ack,sizeof(ack))) < 0)
		{
			printf("client: ack write  error :%d\n",errno);exit(0);
		}

    }

    if (num_last_blk > 0) 
    {   
        if((readn(sockid,in_buf,num_last_blk)) < 0)
        {
        	printf("client: last block error read :%d\n",errno);
        	exit(0);
        }

        no_writen = fwrite(in_buf,sizeof(char),num_last_blk,fp); 
        if (no_writen == 0) 
        {
        	printf("client: last block file write err :%d\n",errno);
        	exit(0);
        }

        if (no_writen != num_last_blk) 
        {
        	printf("client: file write error : no_writen is less 2\n");
        	exit(0);
        }

        if((writen(sockid,(char *)&ack,sizeof(ack))) < 0)
	    {
	    	printf("client :ack write  error  :%d\n",errno);
	    	exit(0);
	    }
    }
	else 
	{
		printf("\n");
	}

	fclose(fp);
}



void SendFile(unsigned int& newsd, string filename)
{
	int no_read ,num_blks,num_blks1,num_last_blk,num_last_blk1, i,fsize,ack,c;
    char fname[MAXLINE], out_buf[MAXSIZE];
    FILE *fp;
      
	no_read = 0;
	num_blks = 0;
	num_last_blk = 0;

	strncpy(fname, filename.c_str(), sizeof(fname));
	fname[sizeof(fname) - 1] = 0;
   
    if((fp = fopen(fname,"r")) == NULL) /*cant open file*/
    {
    	printf("server: file open in read mode error :%d\n",errno);
    }

	printf("server: filename is %s\n",fname);

    fsize = 0;ack = 0;
    while ((c = getc(fp)) != EOF){fsize++;}
	num_blks = fsize / MAXSIZE; 
	num_blks1 = htons(num_blks);
	num_last_blk = fsize % MAXSIZE; 
	num_last_blk1 = htons(num_last_blk);

	if((writen(newsd,(char *)&num_blks1,sizeof(num_blks1))) < 0){printf("server: write error :%d\n",errno);exit(0);}
	if((readn(newsd,(char *)&ack,sizeof(ack))) < 0){printf("server: ack read error :%d\n",errno);exit(0); }          
	if (ntohs(ack) != ACK) {printf("client: ACK not received on file size\n");exit(0);}
	if((writen(newsd,(char *)&num_last_blk1,sizeof(num_last_blk1))) < 0){printf("server: write error :%d\n",errno);exit(0);}
	if((readn(newsd,(char *)&ack,sizeof(ack))) < 0){printf("server: ack read error :%d\n",errno);exit(0); }
	if (ntohs(ack) != ACK) {printf("server: ACK not received on file size\n");exit(0);}
	rewind(fp);


	/* ACTUAL FILE TRANSFER STARTS  BLOCK BY BLOCK*/

	for(i= 0; i < num_blks; i ++) 
	{ 
		no_read = fread(out_buf,sizeof(char),MAXSIZE,fp);
		if (no_read == 0) {printf("server: file read error\n");exit(0);}
		if (no_read != MAXSIZE){printf("server: file read error : no_read is less\n");exit(0);}
		if((writen(newsd,out_buf,MAXSIZE)) < 0){printf("server: error sending block:%d\n",errno);exit(0);}
		if((readn(newsd,(char *)&ack,sizeof(ack))) < 0){printf("server: ack read  error :%d\n",errno);exit(0);}
		if (ntohs(ack) != ACK) {printf("server: ACK not received for block %d\n",i);exit(0);}
	}

	if (num_last_blk > 0) 
	{ 
		no_read = fread(out_buf,sizeof(char),num_last_blk,fp); 
		if (no_read == 0) {printf("server: file read error\n");exit(0);}
		if (no_read != num_last_blk) {printf("server: file read error : no_read is less 2\n");exit(0);}
		if((writen(newsd,out_buf,num_last_blk)) < 0){printf("server: file transfer error %d\n",errno);exit(0);}
		if((readn(newsd,(char *)&ack,sizeof(ack))) < 0){printf("server: ack read  error %d\n",errno);exit(0);}
		if (ntohs(ack) != ACK) {printf("server: ACK not received last block\n");exit(0);}
	}
	else printf("\n");

	fclose(fp);


}