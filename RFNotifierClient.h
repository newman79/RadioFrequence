#ifndef RFSOCKETCLIENT_H
#define RFSOCKETCLIENT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> 		/* close */
#include <netdb.h> 			/* gethostbyname */
#include "RFTools.h"

typedef int 				SOCKET;
typedef struct sockaddr_in 	SOCKADDR_IN;
typedef struct sockaddr 	SOCKADDR;
typedef struct in_addr 		IN_ADDR;

#define INVALID_SOCKET 	-1
#define SOCKET_ERROR 	-1
#define closesocket(s) close(s)

#define CRLF				"\r\n"
#define PORT	 			1977

#define BUF_SIZE			16384

#define DEFAULTCOMMSEP 				string(":")    					// separateur utilisé par défaut pour le découpage des messages reseaux
#define CLIENTREQUESTSEP 			string(" ")    					// separateur utilisé par défaut lors de l'envoi des requetes par les clients
#define REQUEST_LISTCLIENTS 		"listclients"
#define REQUEST_RFSEND 				"rfsend"
#define REQUEST_LISTREMOTES			"rflistremote"
#define REQUEST_GETREMOTEBUTTONS	"rfgetbutton"
#define REQUEST_HELP				"help"


class RFSocketClient
{
public:
	SOCKET sock;
	
	int 	init(const char *address, int port);
	int 	init_connection(const char *address, int port);
	void 	end_connection();
	int		readFromServer(SOCKET sock, char *buffer);
	bool 	sendToServer(const char *buffer);
};

#endif
