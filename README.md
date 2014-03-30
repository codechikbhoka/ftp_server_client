ftp_server_client
=================
FTP Server Client

VIVEK BHARGAV (CSE)
11010172

MAINAK SETHI (CSE)
11010134

The project is an implementation of Active FTP

The project folder contains 2 code files : 
1. Client side code (ftpclient.cpp)
2. Server side code (ftpserver.cpp)

1. Run 'make all' from the project's directory to create executables for ftpclient.cpp and ftpserver.cpp

To run the program on local host copy the either ftpclient executable or ftpserver executable to a different folder. (Both of them should not be in the same directory)
If the client is not on localhost then the folder need not be different.

2. First run the ftpserver executable from its folder by typing './ftpserver'

3. Run the ftpclient executable from its folder and give the address of the server and its port. We have defined the port as '6021'.

for local host type - './ftpclient 127.0.0.1 6021' from the exectuable's directory

4. If the connection gets established , type the command you want to execute at the server side.


The supported features are :
1. get file from the server
for ex - 'get abc.pdf'
 
2. put file at the server 
for ex - 'put abc.pdf'

3. ls (list all the files in the server's present directory)
'ls'

4. pwd (server's present directory)
'pwd'

5. cd ( change the server's present directory)
'cd destination'

6. !ls (all files int the client's present directory)
'!ls'

7. !pwd (client's present directory)
'!pwd'

8. !cd (change client's present directory)
'!cd destination'



Type 'quit' to exit the program





