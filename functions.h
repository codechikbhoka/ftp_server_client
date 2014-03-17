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
#include <sys/stat.h>
#include <time.h>
using namespace std;

#define ACK                   	2
#define NACK                  	3
#define MAXSIZE 				512
#define MAXLINE 				25600
#define SERVER_CONTROL_PORT 	6021

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
		printf("error: socket error : %d\n", errno); exit(0); 
	}

	int yes=1;
	if (setsockopt(sockid, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) 
	{
	    perror("setsockopt");
	    exit(1);
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
		printf("error: bind  error :%d\n", errno); exit(0); 
	}
}

int BindSocketToRandomPort(unsigned int& serverfd)
{
	sockaddr_in channel;
	memset(&channel, 0, sizeof(channel));
	channel.sin_family = AF_INET;
	channel.sin_addr.s_addr = INADDR_ANY;
	bind(serverfd, (sockaddr *) &channel, sizeof(channel));
	socklen_t channellen;
	getsockname(serverfd, (sockaddr*) &channel, &channellen);
	int portno;
	portno = ntohs(channel.sin_port);
	return portno;
}


void ConnectToRemote(unsigned int& sockid, char* REMOTE_HOST_ADDR, int REMOTE_HOST_PORT)
{
	cout << "Connecting to " << REMOTE_HOST_ADDR << ":" << REMOTE_HOST_PORT << endl;
	struct sockaddr_in server_addr; 
	bzero((char *) &server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(REMOTE_HOST_ADDR);
	server_addr.sin_port = htons(REMOTE_HOST_PORT);
	if(connect(sockid ,(struct sockaddr *) &server_addr,sizeof(server_addr)) < 0){printf("error: connect  error :%d\n", errno); exit(0);}
	cout << "Connected to " << REMOTE_HOST_ADDR << ":" << REMOTE_HOST_PORT << endl;
}


void ReceiveFile(unsigned int& sockid, string filename)
{
	int i,ack,getfile,c,len, no_writen, num_blks,num_last_blk;
	FILE *fp;
	char in_buf[MAXSIZE];

    if ((fp = fopen(&filename[0],"w")) == NULL)
    {
    	printf(" error: local open file error \n");
    	exit(0);
    }

	if((readn(sockid,(char *)&num_blks,sizeof(num_blks))) < 0)
	{
		printf("error: read error on nblocks :%d\n",errno);
		exit(0);
	}

	num_blks = ntohs(num_blks);
	ack = ACK;  
	ack = htons(ack);
	if((writen(sockid,(char *)&ack,sizeof(ack))) < 0)
	{
		printf("error: ack write error :%d\n",errno);
		exit(0);
	}
   
	if((readn(sockid,(char *)&num_last_blk,sizeof(num_last_blk))) < 0)
	{
		printf("error: read error :%d on nbytes\n",errno);
		exit(0);
	}
	num_last_blk = ntohs(num_last_blk);  
	if((writen(sockid,(char *)&ack,sizeof(ack))) < 0)
	{
		printf("error: ack write error :%d\n",errno);
		exit(0);
	}

    for(i= 0; i < num_blks; i ++) 
    {
		if((readn(sockid,in_buf,MAXSIZE)) < 0)
		{
			printf("error: block error read: %d\n",errno);exit(0);
		}

		no_writen = fwrite(in_buf,sizeof(char),MAXSIZE,fp);
		if (no_writen == 0)
		{
			printf("error: file write error\n");exit(0);
		}
		if (no_writen != MAXSIZE) 
		{
			printf("error: file write  error : no_writen is less\n");exit(0);
		}

		if((writen(sockid,(char *)&ack,sizeof(ack))) < 0)
		{
			printf("error: ack write  error :%d\n",errno);exit(0);
		}

    }

    if (num_last_blk > 0) 
    {   
        if((readn(sockid,in_buf,num_last_blk)) < 0)
        {
        	printf("error: last block error read :%d\n",errno);
        	exit(0);
        }

        no_writen = fwrite(in_buf,sizeof(char),num_last_blk,fp); 
        if (no_writen == 0) 
        {
        	printf("error: last block file write err :%d\n",errno);
        	exit(0);
        }

        if (no_writen != num_last_blk) 
        {
        	printf("error: file write error : no_writen is less 2\n");
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
    	printf("error: client requested for a non existing file :%d\n",errno);
    }

    fsize = 0;ack = 0;
    while ((c = getc(fp)) != EOF){fsize++;}
	num_blks = fsize / MAXSIZE; 
	num_blks1 = htons(num_blks);
	num_last_blk = fsize % MAXSIZE; 
	num_last_blk1 = htons(num_last_blk);

	if((writen(newsd,(char *)&num_blks1,sizeof(num_blks1))) < 0){printf("error: write error :%d\n",errno);exit(0);}
	if((readn(newsd,(char *)&ack,sizeof(ack))) < 0){printf("error: ack read error :%d\n",errno);exit(0); }          
	if (ntohs(ack) != ACK) {printf("error: ACK not received on file size\n");exit(0);}
	if((writen(newsd,(char *)&num_last_blk1,sizeof(num_last_blk1))) < 0){printf("error: write error :%d\n",errno);exit(0);}
	if((readn(newsd,(char *)&ack,sizeof(ack))) < 0){printf("error: ack read error :%d\n",errno);exit(0); }
	if (ntohs(ack) != ACK) {printf("error: ACK not received on file size\n");exit(0);}
	rewind(fp);


	/* ACTUAL FILE TRANSFER STARTS  BLOCK BY BLOCK*/

	for(i= 0; i < num_blks; i ++) 
	{ 
		no_read = fread(out_buf,sizeof(char),MAXSIZE,fp);
		if (no_read == 0) {printf("error: file read error\n");exit(0);}
		if (no_read != MAXSIZE){printf("error: file read error : no_read is less\n");exit(0);}
		if((writen(newsd,out_buf,MAXSIZE)) < 0){printf("error: error sending block:%d\n",errno);exit(0);}
		if((readn(newsd,(char *)&ack,sizeof(ack))) < 0){printf("error: ack read  error :%d\n",errno);exit(0);}
		if (ntohs(ack) != ACK) {printf("error: ACK not received for block %d\n",i);exit(0);}
	}

	if (num_last_blk > 0) 
	{ 
		no_read = fread(out_buf,sizeof(char),num_last_blk,fp); 
		if (no_read == 0) {printf("error: file read error\n");exit(0);}
		if (no_read != num_last_blk) {printf("error: file read error : no_read is less 2\n");exit(0);}
		if((writen(newsd,out_buf,num_last_blk)) < 0){printf("error: file transfer error %d\n",errno);exit(0);}
		if((readn(newsd,(char *)&ack,sizeof(ack))) < 0){printf("error: ack read  error %d\n",errno);exit(0);}
		if (ntohs(ack) != ACK) {printf("error: ACK not received last block\n");exit(0);}
	}
	else printf("\n");

	fclose(fp);


}


string intTOstring(int a)
{
	stringstream ss;
	ss << a;
	string str = ss.str();
	return str;

}

int stringTOint(string a)
{
	istringstream ss(a);
	int k;
	ss >> k;
	return k;
}


int recvallbinary(int serverfd, FILE *fd)
{
	unsigned char buf[10001];
	int bytesRead=0;
	int len=0;
	while((bytesRead = recv(serverfd,buf,10000,0)) >0){
		len+=bytesRead;
		fwrite(buf,1,bytesRead,fd);
	}
	if(bytesRead < 0){
		cerr<<"Error Occurred";
		return -1;
	}else{
 
		return len;
	}
}
 
int send_all(int socket,const void *buffer, size_t length) {
    size_t i = 0;
    for (i = 0; i < length;){
    	int bytesSent = send(socket, buffer, length - i,MSG_NOSIGNAL);
    	if(bytesSent==-1){
    		return errno;
    	}else{
    		i+=bytesSent;
    	}
    }
    return 0;
}

int sendallbinary(int serverfd, FILE *fd,int size)
{
	unsigned char buf[100001];
	int bytesSent=0;
	while(size>0){
		int bytesRead = fread(buf,1,100000,fd);
		int stat = send_all(serverfd,buf,bytesRead);
		if(stat != 0 ){

			cout<<"ERROR IN SENDING"<<endl;
			cout << strerror(errno) << endl;
			return -1;
		}
 
		size = size - bytesRead;
 
	}
	return 0;	
}

