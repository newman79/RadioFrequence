#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <sys/time.h>
#include <time.h>
#include "RFNotifierClient.h"

int RFSocketClient::init(const char *address, int port)
{
	this->sock = this->init_connection(address, port);
	if (this->sock == 0)
		return 1;
		
	trace (0,"Init connection to server : OK");
	char buffer[BUF_SIZE];

	fd_set rdfs;

	struct hostent * hostinfo = gethostbyname("localhost");
	string host_date;
	if (hostinfo == NULL)	host_date="UNKNOWN";
	else	 				host_date=(char*)hostinfo->h_name;
	host_date += "RFReceptNotifier";
	host_date += "_";	host_date += nowInMicroSecondToString();
	string str = "Registered at server with following client id : " + host_date;
	trace (0, str.c_str());
	
	this->sendToServer(host_date.c_str()); /* send clientId of this process to server */
	return 0;
}

/*------------------------------------------------------------------------------------------------------------------------------*/
int RFSocketClient::init_connection(const char *address, int port)
{
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN sin = { 0 };
	struct hostent *hostinfo;

	if(sock == INVALID_SOCKET)
	{
	  perror("socket()");
	  exit(errno);
	}

	hostinfo = gethostbyname(address);
	if (hostinfo == NULL)
	{
		string str = "Unknown host :" ;
		str += address;
		trace (1,str.c_str());
		exit(EXIT_FAILURE);
	}

	sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr;
	sin.sin_port = htons(port);
	sin.sin_family = AF_INET;

	if(connect(sock,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		perror("connect()");
		return 0;
	}
	return sock;
}

/*------------------------------------------------------------------------------------------------------------------------------*/
void RFSocketClient::end_connection()
{
	closesocket(this->sock);
}

/*------------------------------------------------------------------------------------------------------------------------------*/
bool RFSocketClient::sendToServer(const char *buffer)
{
	int messageSize = strlen(buffer);
	if (messageSize >= BUF_SIZE)
	{
		string toTrace = "Cannot send message because it is too long : ";
		toTrace += messageSize;
		toTrace += " bytes";
		trace(3, toTrace.c_str());
	}
	if(send(this->sock, buffer, strlen(buffer), 0) < 0)	return false;
	else 												return true;
}
