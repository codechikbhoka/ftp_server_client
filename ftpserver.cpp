#include "functions.h"

bool FTP_Session(unsigned int newsd, char CLIENT_HOST_ADDR[]);

int main()  
{

	unsigned int sockid, newsd, pid,clilen;
	struct sockaddr_in client_addr;   

	CreateNewSocket(sockid);  //function defined in functions.h, initialise a new socket
	BindSocketToLocalPort(sockid,SERVER_CONTROL_PORT); //Bind sockid to SERVER_CONTROL_PORT
	listen(sockid,5);  //start listening on sockid
	cout << "server: Starting Server" << endl;
	cout << "server: waiting for connection..." << endl;


	while(true) 
	{ 
		//Accept a New Connection and store a new server descriptor
		if ((newsd = accept(sockid ,(struct sockaddr *) &client_addr, &clilen)) < 0)
		{
			printf("server: accept  error :%d\n", errno); exit(0);
		}

		//fork a new child here and carry out an FTP session with that child
		if ( (pid=fork()) == 0) 
		{
			close(sockid);  //No need for parent's sockid in child

			char* CLIENT_HOST_ADDR = inet_ntoa(client_addr.sin_addr); //get the ip of client who connected
			cout << "FTP Session Initiated For " << CLIENT_HOST_ADDR << endl;
			while(FTP_Session(newsd, CLIENT_HOST_ADDR));  // call FTP_Session again after completion of one valid command
			cout << "FTP Session Dropped For " << CLIENT_HOST_ADDR << endl;
			close (newsd);
			exit(0);
		}

		//Parent continue ...
		close(newsd);
	}

	return 0;
}   

bool FTP_Session(unsigned int newsd, char* CLIENT_HOST_ADDR)
{      
    int i,msg_ok,c;

    string command = RecvString(newsd);  //Receive Command from client like "get", "put", "ls" etc
	cout << "server: client requested result for command : " << command << endl;

	if (command == "ls" || command == "pwd")
	{
		string result = exec(command) ;  // Execute command and store result in result
		SendString(newsd,result);  // send result of command to client
		return true;
	}
	else if (command == "cd")
	{
		if(chdir(&RecvString(newsd)[0]) == 0)  //Receive directory to change to, execute cd command
		{
			cout << "server: directory changed" << endl;
		}
		else
		{
			printf("server: chdir error :%d\n",errno);
		}
		return true;
	}
	else if (command == "quit")
    {
    	return false;
    }
	else if (command == "get")
	{
	    string filename = RecvString(newsd);  // Get FileName
	    string ClientRandPort_str = RecvString(newsd);  // Get the port at which client is listening -> (Client Data Port)
	    int ClientRandPort_int = stringTOint(ClientRandPort_str);



	    unsigned int sockdataid;  //declare id for server's data socket
    	CreateNewSocket(sockdataid);  // Create a socket on this id
		int randport = 40000 + rand()%200;  // generate a random port
		BindSocketToLocalPort(sockdataid,randport);  // Bind server's data socket to this randport


		cout << CLIENT_HOST_ADDR << " requested file " << filename << endl;
		ConnectToRemote(sockdataid, CLIENT_HOST_ADDR, ClientRandPort_int);  // Connect to Client's Data Port

		SendFile(sockdataid, filename);  // Send File to client, this function is defined in functions.h
		cout << "server: " << filename << " sent to " << CLIENT_HOST_ADDR << endl;
		close(sockdataid);
	    
		return true;
	}
	else if (command == "put")
	{
		string filename = RecvString(newsd);  // Get FileName
		string ClientRandPort_str = RecvString(newsd);// Get the port at which client is listening -> (Client Data Port)
		int ClientRandPort_int = stringTOint(ClientRandPort_str);



		unsigned int sockdataid;  //declare id for server's data socket
    	CreateNewSocket(sockdataid);  // Create a socket on this id
		int randport = 40000 + rand()%200;  // generate a random port
		BindSocketToLocalPort(sockdataid,randport);  // Bind server's data socket to this randport

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