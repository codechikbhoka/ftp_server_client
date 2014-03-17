#include "functions.h"

bool FTP_Session(unsigned int newsd, char CLIENT_HOST_ADDR[]);

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

			char* CLIENT_HOST_ADDR = inet_ntoa(client_addr.sin_addr);
			cout << "FTP Session Initiated For " << CLIENT_HOST_ADDR << endl;
			while(FTP_Session(newsd, CLIENT_HOST_ADDR));
			cout << "FTP Session Dropped For " << CLIENT_HOST_ADDR << endl;
			close (newsd);
			exit(0);
		}

		close(newsd);
	}

	return 0;
}   

bool FTP_Session(unsigned int newsd, char* CLIENT_HOST_ADDR)
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
    	return false;
    }
	else if (command == "get")
	{
	    string filename = RecvString(newsd);
	    string ClientRandPort_str = RecvString(newsd);
	    int ClientRandPort_int = stringTOint(ClientRandPort_str);
	    int sockdataid;
    	CreateNewSocket(sockdataid);
		int randport = 40000 + rand()%200;
		BindSocketToLocalPort(sockdataid,randport);
		cout << CLIENT_HOST_ADDR << " requested file " << filename << endl;
		cout << "sockdataid  is  "<< sockdataid << endl;
		ConnectToRemote(sockdataid, CLIENT_HOST_ADDR, ClientRandPort_int);

		FILE* f = fopen(&filename[0],"rb");
		struct stat st;
		stat(&filename[0], &st);
		int size = st.st_size;
		sendallbinary(sockdataid,f,size);
		//SendFile(sockdataid, filename);
		cout << "server: " << filename << " sent to " << CLIENT_HOST_ADDR << endl;
		close(sockdataid);
	    
		return true;
	}
	else if (command == "put")
	{
		string filename = RecvString(newsd);
		string ClientRandPort_str = RecvString(newsd);
		int ClientRandPort_int = stringTOint(ClientRandPort_str);
		unsigned int sockdataid;
    	CreateNewSocket(sockdataid);
		int uselessVar = BindSocketToRandomPort(sockdataid);
		ConnectToRemote(sockdataid, CLIENT_HOST_ADDR, ClientRandPort_int);
		ReceiveFile(sockdataid, filename);
		cout << "server: " << CLIENT_HOST_ADDR << " sent file " << filename << endl;
		close(sockdataid);
		return true;
	}
	else
	{
		cout << "server: An invalid FTP Command" << endl;
		return false;
    }

}