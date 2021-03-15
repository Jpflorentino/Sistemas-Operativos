#ifndef _myinet_h
#define _myinet_h

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>


#include <stdlib.h>
#include <stdio.h>


/* Endere�os IP das maquinas de teste do ISEL/DEEC */
#define LOCALHOST	    "127.0.0.1"
#define DEFAULT_HOST	LOCALHOST


/* Porto utilizado pelo servidor */
#define DEFAULT_PORT		8000



#define DIM_BUFFER		512


#endif
