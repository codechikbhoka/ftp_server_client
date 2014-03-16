#include "functions.h"


int main(int argc,char *argv[])
{


	unsigned int sockid, newsockid, sockdataid, newdatasd, servlen;
	int i,ack,msg,getfile,msg_2,c,len,pid2;
	int no_writen,start_xfer, num_blks,num_last_blk;
	struct sockaddr_in my_addr, server_addr; 
	FILE *fp; 
	char* SERVER_HOST_ADDR;



	if(argc != 2) {printf("error: usage : ./ftpclient Server_IP_Address\n"); exit(0);}
	no_writen = 0;
	num_blks = 0;
	num_last_blk = 0;
	SERVER_HOST_ADDR = argv[1];


	CreateNewSocket(sockid);
	BindSocketToLocalPort(sockid,CLIENT_CONTROL_PORT);
	ConnectToRemote(sockid, SERVER_HOST_ADDR, SERVER_CONTROL_PORT);

	
	while(1)
	{

			cout << "\nftpclient> : " ;
			string command, filename;
			cin >> command;			

			if (command == "ls" || command == "pwd")
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
		    	close(sockid);
		    	break;
		    }
			else if (command ==  "get")
			{
				cin >> filename;
			    SendString(sockid,command);
			    SendString(sockid,filename);					    
		    	CreateNewSocket(sockdataid);
				BindSocketToLocalPort(sockdataid,CLIENT_DATA_PORT);
				listen(sockdataid,5);
				if ((newdatasd = accept(sockdataid ,(struct sockaddr *) &server_addr, &servlen)) < 0){printf("client: accept  error :%d\n", errno); exit(0);}
		    	ReceiveFile(newdatasd, filename);
		    	printf("client: FILE TRANSFER COMPLETE\nftpclient> : ");
				close(sockdataid);
		    }
		    else if (command ==  "put")
			{
				cin >> filename;
			    SendString(sockid,command);
			    SendString(sockid,filename);
				CreateNewSocket(sockdataid);
				BindSocketToLocalPort(sockdataid,CLIENT_DATA_PORT);
				listen(sockdataid,5);
				if ((newdatasd = accept(sockdataid ,(struct sockaddr *) &server_addr, &servlen)) < 0){printf("client: accept  error :%d\n", errno); exit(0);}
		    	SendFile(newdatasd,filename);
		    	printf("client: FILE TRANSFER COMPLETE\n");
				close(sockdataid);			    					    
					   					
		    }
		    else
		    {
		    	cout << "client : An invalid FTP Command" << endl;
		    }

	}
    
    close(sockid);

    return 0;
}


