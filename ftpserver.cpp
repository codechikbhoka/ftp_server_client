#include "functions.h"

bool FTP_Session(unsigned int newsd);

int main()  
{

	unsigned int sockid, newsd, pid,clilen;
	struct sockaddr_in client_addr;   

	CreateNewSocket(sockid);
	BindSocketToLocalPort(sockid,SERVER_CONTROL_PORT);
	listen(sockid,5);
	cout << "server: Starting Server" << endl;
	cout << "server: waiting for connection..." << endl;


	while(true) 
	{ 
		//Accept a New Connection and store a new server descriptor
		if ((newsd = accept(sockid ,(struct sockaddr *) &client_addr, &clilen)) < 0)
		{printf("server: accept  error :%d\n", errno); exit(0);}

		if ( (pid=fork()) == 0) 
		{
			close(sockid);
			while(FTP_Session(newsd));
			close (newsd);
			exit(0);
		}

		close(newsd);
	}

	return 0;
}   

bool FTP_Session(unsigned int newsd)
{       
    int i,fsize,fd,msg_ok,fail,fail1,req,c,ack,pid2;

    string command = RecvString(newsd);
	cout << "server: client requested result for command : " << command << endl;

	if (command == "ls" || command == "pwd")
	{
		string msg_ok = exec(command) ;
		SendString(newsd,msg_ok);
		return true;
	}
	else if (command == "cd")
	{
		if(chdir(&RecvString(newsd)[0]) == 0)
		{
			cout << "server: directory changed" << endl;
		}
		else
		{
			printf("server: chdir error :%d\n",errno);
		}
		return true;
	}
	else if (command == "quit" || command == "exit")
    {
    	cout << "server: Client Dropped" << endl;
    	return true;
    }
	else if (command == "get")
	{
	    string filename = RecvString(newsd);
	    unsigned int sockdataid;
    	CreateNewSocket(sockdataid);
		BindSocketToLocalPort(sockdataid,SERVER_DATA_PORT);
		ConnectToRemote(sockdataid, CLIENT_HOST_ADDR, CLIENT_DATA_PORT);
		SendFile(sockdataid, filename);
		printf("server: FILE TRANSFER COMPLETE\n");
		shutdown(sockdataid,2);
	    
		return true;
	}
	else if (command == "put")
	{
		string filename = RecvString(newsd);
		unsigned int sockdataid;
    	CreateNewSocket(sockdataid);
		BindSocketToLocalPort(sockdataid,SERVER_DATA_PORT);
		ConnectToRemote(sockdataid, CLIENT_HOST_ADDR, CLIENT_DATA_PORT);
		ReceiveFile(sockdataid, filename);
		printf("server: FILE TRANSFER COMPLETE\n");
		shutdown(sockdataid,2);
		return true;
	}
	else
	{
		cout << "server: An invalid FTP Command" << endl;
		return false;
    }

}