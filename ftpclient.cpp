#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <vector>

using namespace std;

#define SERVER_CONTROL_PORT 6021
#define SERVER_DATA_PORT 6020
#define CLIENT_CONTROL_PORT 6081
#define CLIENT_DATA_PORT 6080
#define MAXSIZE 512
#define ACK                   2
#define NACK                  3
#define REQUESTFILE           100
#define REQUESTLISTDIRECTORY  101
#define COMMANDNOTSUPPORTED   150
#define COMMANDSUPPORTED      160
#define BADFILENAME           200
#define FILENAMEOK            400
#define STARTTRANSFER         500

int readn(int sd,char *ptr,int size);
int writen(int sd,char *ptr,int size);
void SendString(int sd, string pkt);
string RecvString(int csd);
string exec(string cmd) ;

int main(int argc,char *argv[])
{


	int sockid, newsockid,i,getfile,ack,msg,msg_2,c,len;
	int no_writen,start_xfer, num_blks,num_last_blk;
	struct sockaddr_in my_addr, server_addr; 
	FILE *fp; 
	char in_buf[MAXSIZE];
	char* SERVER_HOST_ADDR;



	if(argc != 2) {printf("error: usage : ./ftpclient Server_IP_Address\n"); exit(0);}
	no_writen = 0;
	num_blks = 0;
	num_last_blk = 0;
	SERVER_HOST_ADDR = argv[1];


	printf("client: creating socket\n");
	if ((sockid = socket(AF_INET,SOCK_STREAM,0)) < 0){ printf("client: socket error : %d\n", errno); exit(0);}
	printf("client: binding my local socket\n");
	bzero((char *) &my_addr,sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	my_addr.sin_port = htons(CLIENT_CONTROL_PORT);
	if (bind(sockid ,(struct sockaddr *) &my_addr,sizeof(my_addr)) < 0){printf("client: bind  error :%d\n", errno); exit(0);}
	printf("client: starting connect\n");
	bzero((char *) &server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_HOST_ADDR);
	server_addr.sin_port = htons(SERVER_CONTROL_PORT);
	if(connect(sockid ,(struct sockaddr *) &server_addr,sizeof(server_addr)) < 0){printf("client: connect  error :%d\n", errno); exit(0);}


	/* Once we are here, we've got a connection to the server */

	/* tell server that we want to get a file */
	while(1)
	{

			cout << "\nftpclient> : " ;
			string command, filename;
			cin >> command;			

			if (command ==  "get" || command == "put")
			{
						cin >> filename;
						cout << "filename is |" << filename << endl;len = filename.length();
		    			getfile = htons(REQUESTFILE);
					    printf("client: sending command 'get' to ftp server\n");
					    SendString(sockid,command);
					    //if((writen(sockid,(char *)&getfile,sizeof(getfile))) < 0){printf("client: write  error :%d\n", errno); exit(0);} 

					    /* want for go-ahead from server */
					    msg = 0;  
					    if((readn(sockid,(char *)&msg,sizeof(msg)))< 0){printf("client: read  error :%d\n", errno); exit(0); }
					    msg = ntohs(msg);   
					    if (msg==COMMANDNOTSUPPORTED) {printf("client: server refused command. goodbye\n");exit(0);}
					    else printf("client: server replied %d, command supported\n",msg);

		    		/* send file name to server */
					    printf("client: sending filename\n");
					       if ((writen(sockid,&filename[0],len))< 0)
					         {printf("client: write  error :%d\n", errno); exit(0);}
					    /* see if server replied that file name is OK */
					    msg_2 = 0;
					    if ((readn(sockid,(char *)&msg_2,sizeof(msg_2)))< 0)
					        {printf("client: read  error :%d\n", errno); exit(0); }   
					    msg_2 = ntohs(msg_2);
					    if (msg_2 == BADFILENAME) 
					    {
					       printf("client: server reported bad file name. goodbye.\n");
					       exit(0);
					    }
					     else
					         printf("client: server replied %d, filename OK\n",msg_2);


					  /* CLIENT KNOWS SERVER HAS BEEN ABLE TO OPEN THE FILE IN READ 
					     MODE AND IS ASKING FOR GO-AHEAD*/
					  /* CLIENT NOW OPENS A COPY OF THE FILE IN WRITE MODE AND SENDS 
					     THE GOAHEAD TO SERVER*/ 
					    printf("client: sending start transfer command\n");
					    start_xfer = STARTTRANSFER;
					    start_xfer = htons(start_xfer);
					    if ((writen(sockid,(char *)&start_xfer,sizeof(start_xfer)))< 0)
					           {printf("client: write  error :%d\n", errno); exit(0);
					           }
					    if ((fp = fopen(&filename[0],"w")) == NULL)
					        {printf(" client: local open file error \n");exit(0);}
					            
					    

					  /*NOW THE CLIENT IS READING INFORMATION FROM THE SERVER REGARDING HOW MANY
					    FULL BLOCKS OF SIZE MAXSIZE IT CAN EXPECT. IT ALSO RECEIVES THE NUMBER
					   OF BYTES REMAINING IN THE LAST PARTIALLY FILLED BLOCK, IF ANY */  

					     if((readn(sockid,(char *)&num_blks,sizeof(num_blks))) < 0)
					             {printf("client: read error on nblocks :%d\n",errno);exit(0);}
					     num_blks = ntohs(num_blks);
					     printf("client: server responded: %d blocks in file\n",num_blks);
					     ack = ACK;  
					     ack = htons(ack);
					     if((writen(sockid,(char *)&ack,sizeof(ack))) < 0)
					        {printf("client: ack write error :%d\n",errno);exit(0);
					        }

					   
					     if((readn(sockid,(char *)&num_last_blk,sizeof(num_last_blk))) < 0)
					             {printf("client: read error :%d on nbytes\n",errno);exit(0);}
					     num_last_blk = ntohs(num_last_blk);  
					     printf("client: server responded: %d bytes last blk\n",num_last_blk);
					     if((writen(sockid,(char *)&ack,sizeof(ack))) < 0)
					        {printf("client: ack write error :%d\n",errno);exit(0);
					        }


					  /* BEGIN READING BLOCKS BEING SENT BY SERVER */
					  printf("client: starting to get file contents\n");
					    for(i= 0; i < num_blks; i ++) {
					      if((readn(sockid,in_buf,MAXSIZE)) < 0)
						  {printf("client: block error read: %d\n",errno);exit(0);}
					      no_writen = fwrite(in_buf,sizeof(char),MAXSIZE,fp);
					      if (no_writen == 0) {printf("client: file write error\n");exit(0);}
					      if (no_writen != MAXSIZE) 
					         {printf("client: file write  error : no_writen is less\n");exit(0);}
					      /* send an ACK for this block */
					      if((writen(sockid,(char *)&ack,sizeof(ack))) < 0)
					         {printf("client: ack write  error :%d\n",errno);exit(0);}
					      printf(" %d...",i);
					      }


					/*IF THERE IS A LAST PARTIALLY FILLED BLOCK, READ IT */

					    if (num_last_blk > 0) 
					    {
					        printf("%d\n",num_blks);      
					        if((readn(sockid,in_buf,num_last_blk)) < 0)
					           {printf("client: last block error read :%d\n",errno);exit(0);}
					        no_writen = fwrite(in_buf,sizeof(char),num_last_blk,fp); 
					        if (no_writen == 0) 
					          {printf("client: last block file write err :%d\n",errno);exit(0);}
					        if (no_writen != num_last_blk) 
					        {printf("client: file write error : no_writen is less 2\n");exit(0);}
					        if((writen(sockid,(char *)&ack,sizeof(ack))) < 0)
						         {printf("client :ack write  error  :%d\n",errno);exit(0);}
					    }
						else 
						{
							printf("\n");
						}

						fclose(fp);
						printf("client: FILE TRANSFER COMPLETE\n");
		    }
		    else if (command == "ls" || command == "pwd")
		    {
		    			SendString(sockid,command);
					    string reply = RecvString(sockid);
				    	cout << reply << endl;   	
		    }
		    else if (command == "!ls" || command == "!pwd")
		    {
		    	command.erase(0,1);
		    	cout << exec(command) << endl;
		    }
		    else if (command == "cd")
		    {
		    	SendString(sockid,command);
		    	string nxtDir;
		    	cin >> nxtDir;
		    	SendString(sockid,nxtDir);
		    }
		    else if (command == "!cd")
			{
				string nxtDir;
		    	cin >> nxtDir;
				chdir(&nxtDir[0]);
				cout << "client : directory changed" << endl;
			}
		    else if (command == "quit" || command == "exit")
		    {
		    	SendString(sockid,command);
		    	break;
		    }
	}
    
    close(sockid);

    return 0;
}	     

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

void SendString(int sd, string pkt)
{
	uint32_t msg_length = pkt.size();
	uint32_t sndMsgLength = htonl(msg_length); // Ensure network byte order
	send(sd,&sndMsgLength ,sizeof(uint32_t) ,MSG_CONFIRM); // Send the message length
	send(sd,pkt.c_str() ,msg_length ,MSG_CONFIRM); // Send the message data 
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

string RecvString(int csd)
{
	uint32_t msgLength;
	recv(csd,&msgLength,sizeof(uint32_t),0); // Receive the message length
	msgLength = ntohl(msgLength); // Ensure host system byte order

	vector<uint8_t> pkt; // Allocate a receive buffer
	pkt.resize(msgLength,0x00);

	recv(csd,&(pkt[0]),msgLength,0); // Receive the message data
	string tmp(pkt.begin(),pkt.end());
	//tmp.assign(&(pkt[0]),pkt.size()); // Convert message data to a string

	return tmp;
}