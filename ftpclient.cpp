#include "functions.h"


int main(int argc,char *argv[])
{


	unsigned int sockid, newsockid, sockdataid, newdatasd, servlen;
	int i,c;
	struct sockaddr_in my_addr, server_addr; 
	FILE *fp; 
	char *SERVER_HOST_ADDR, *SERVER_CONT_PORT;

	if(argc != 3) {printf("error: usage : ./ftpclient Server_IP_Address Server_Port\n"); exit(0);}
	SERVER_HOST_ADDR = argv[1];
	SERVER_CONT_PORT = argv[2];


	CreateNewSocket(sockid);  //Create new socket at sockid
	int uselessVAR = BindSocketToRandomPort(sockid);  // bind socket to a random free port
	ConnectToRemote(sockid, SERVER_HOST_ADDR, atoi(SERVER_CONT_PORT));  // Connect to server

	
	while(1)
	{

			cout << "\nftpclient> : " ;
			string command, filename;
			cin >> command;			// get input from keyboard

			if (command == "ls" || command == "pwd")
		    {
		    			SendString(sockid,command);  // send command to server
					    string reply = RecvString(sockid);  // receive result from server
				    	cout << reply << endl;   	
		    }
		    else if (command == "!ls" || command == "!pwd")
		    {
		    	command.erase(0,1);  // remove ! from begining
		    	cout << exec(command) << endl;  //execute command
		    }
		    else if (command == "cd")
		    {
		    	SendString(sockid,command);  // send command to server
		    	string nxtDir;
		    	cin >> nxtDir;  // get dir to change to
		    	SendString(sockid,nxtDir);  // send the desired directory to server
		    }
		    else if (command == "!cd")
			{
				string nxtDir;
		    	cin >> nxtDir;
				chdir(&nxtDir[0]);
				cout << "client : directory changed" << endl;
			}
		    else if (command == "quit")
		    {
		    	SendString(sockid,command);
		    	close(sockid);
		    	break;
		    }
			else if (command ==  "get")
			{
				cin >> filename;
			    SendString(sockid,command);  // send "get" command to server
			    SendString(sockid,filename);	// Send filename to server
			    

		    	CreateNewSocket(sockdataid);

		    	srand(time(NULL));                                /*  CREATE    */
		    	int randport = 40000 + rand()%200;				 /* A RANDOM   */
		    	string Rand_Port = intTOstring(randport);       /*  PORT      */

		    	BindSocketToLocalPort(sockdataid,randport);
		    	SendString(sockid,Rand_Port);		 // send rand port to server   	
				listen(sockdataid,5); // start listening


				if ((newdatasd = accept(sockdataid ,(struct sockaddr *) &server_addr, &servlen)) < 0)
				{
					printf("client: accept  error :%d\n", errno); exit(0);
				}

		    	ReceiveFile(newdatasd, filename);
		    	printf("client: FILE TRANSFER COMPLETE\nftpclient> : ");
				close(sockdataid);
		    }
		    else if (command ==  "put")
			{
				cin >> filename;
			    SendString(sockid,command);  // send "get" command to server
			    SendString(sockid,filename);	// Send filename to server


				CreateNewSocket(sockdataid);

		    	srand(time(NULL));                                /*  CREATE    */
		    	int randport = 40000 + rand()%200;				 /* A RANDOM   */
		    	string Rand_Port = intTOstring(randport);       /*  PORT      */

				BindSocketToLocalPort(sockdataid,randport);
		    	SendString(sockid,Rand_Port);		 // send rand port to server   	
				listen(sockdataid,5); // start listening


				if ((newdatasd = accept(sockdataid ,(struct sockaddr *) &server_addr, &servlen)) < 0)
				{
					printf("client: accept  error :%d\n", errno); exit(0);
				}

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