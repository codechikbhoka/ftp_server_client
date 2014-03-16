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