
/*
 * change History
 *   07sep97: created
 *   24Sep97: added sockinfo, senddata, recvdata
 *	 Jan-Feb00: modified for NT/UNIX transparency
 */
#ifdef __WIN_MSVC__
#include <winsock2.h>
#include <process.h>
#include <io.h>
#include <stdio.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#endif

#include "socklib.h"

#define SERIALIZE_LISTEN

#ifdef __WIN_MSVC__
static struct {
	int errorNumber;
	char *errorString;
} sockerrlist [] = {
	{10013, "Permission denied"},
	{10048, "Address already in use"},
	{10049, "Cannot assign requested address"},
	{10047, "Address family not supported by protocol family"},
	{10037, "Operation already in progress"},
	{10053, "Software caused connection abort"},
	{10061, "Connection refused"},
	{10054, "Connection reset by peer"},
	{10039, "Destination address required"},
	{10014, "Bad address"},
	{10064, "Host is down"},
	{10065, "No route to host"},
	{10036, "Operation now in progress"},
	{10004, "Interrupted function call"},
	{10022, "Invalid argument"},
	{10056, "Socket is already connected"},
	{10024, "Too many open files"},
	{10040, "Message too long"},
	{10050, "Network is down"},
	{10052, "Network dropped connection on reset"},
	{10051, "Network is unreachable"},
	{10055, "No buffer space available"},
	{10042, "Bad protocol option"},
	{10057, "Socket is not connected"},
	{10038, "Socket operation on non-socket"},
	{10045, "Operation not supported"},
	{10046, "Protocol family not supported"},
	{10067, "Too many processes"},
	{10043, "Protocol not supported"},
	{10041, "Protocol wrong type for socket"},
	{10058, "Cannot send after socket shutdown"},
	{10044, "Socket type not supported"},
	{10060, "Connection timed out"},
	{10109, "Class type not found"},
	{10035, "Resource temporarily unavailable"},
	{11001, "Host not found"},
	{10093, "Successful WSAStartup not yet performed"},
	{11004, "Valid name; no data record of requested type"},
	{11003, "Non-recoverable error"},
	{10091, "Network subsystem is unavailable"},
	{11002, "Non-authoritative host not found"},
	{10092, "WINSOCK.DLL version out of range"},
	{10094, "Graceful shutdown in progress"},
	{0, NULL}
	}; 
#endif

void sockerror(char *msg) {
#ifdef __WIN_MSVC__
	int err = WSAGetLastError();
	int i;
	for (i = 0; sockerrlist[i].errorNumber; i++)
		if (sockerrlist[i].errorNumber == err) {
			fprintf(stderr, "%s: (%d) %s\n", msg, err, sockerrlist[i].errorString);
			return;
		}
#endif
	perror(msg);
}



static void SetOptions(int serverSocket)
{
    int on = 0,
      port = 0,
      status = 0;
  
      /*
       *      * turn off bind address checking, and allow
       *      * port numbers to be reused - otherwise
       *      * the TIME_WAIT phenomenon will prevent
       *      * binding to these address.port combinations
       *      * for (2 * MSL) seconds.
       *      */
  
      if (-1 == serverSocket)
    {
              perror("socket()");
              exit(1);
    }
  
      on = 1;
  
      status = setsockopt(serverSocket, SOL_SOCKET,
			                  SO_REUSEADDR,
			          (const char *) &on, sizeof(on));
  
      if (-1 == status)
    {
              perror("setsockopt(...,SO_REUSEADDR,...)");
    }
  
      /*
       *      * when connection is closed, there is a need
       *      * to linger to ensure all data is
       *      * transmitted, so turn this on also
       *      */
    {
              struct linger linger = { 1 
	      };
      
              linger.l_onoff = 0;
              linger.l_linger = 30;
              status = setsockopt(serverSocket,
				                  SOL_SOCKET, SO_LINGER,
				                  (const char *) &linger,
				                  sizeof(linger));
      
              if (-1 == status)
	{
	              perror("setsockopt(...,SO_LINGER,...)");
	}
    }
}



/*
 * This is a dual purpose routine.
 * - if host is not NULL, connect as a client to the given host and port.
 * - if host is NULL, bind a socket to the given port.
 * In either case, return a valid socket or -1.
 */
int sockopen(char *host, int port, int *p_assigned_port)
{
    int sd; 
    struct sockaddr_in sin;  /* was static PFHIT */
    struct hostent *hp;
    size_t namelen = sizeof(sin);
    int assigned_port = port;

    //IFVERB printf("((((())))) beginning sockopen (%d)\n", errno);
    //errno = 0;
    //IFVERB printf("((((())))) beginning sockopen (%d)\n", errno);

	/*
	if (host==NULL)
	  {
	    port = 0;
	  }
	*/

    /* get an internet domain socket */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return -1;

    /* complete the socket structure */
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons((short)port);
    if (host != NULL) {
        /* get the IP address of the requested host */
        if ((hp = gethostbyname(host)) == 0)
            return -1;
        sin.sin_addr.s_addr = ((struct in_addr *) (hp->h_addr))->s_addr;
        if (connect(sd, (struct sockaddr *) &sin, sizeof(sin)) == -1)
            return -1;
    } else {                    /* server */
	    int one = 1;
        /* avoid "bind: socket already in use" msg */
        if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof(one)) < 0)
            return -1;
        /* bind the socket to the port number */

	{
	    if (bind(sd, (struct sockaddr *) &sin, sizeof(sin)) == -1)
	      return -1;
	  }
	
	  //if (port!=0)
	  {
	    /* find out what port was assigned */
	    namelen = sizeof(sin);
	    if( getsockname( sd, (struct sockaddr *) &sin, &namelen) < 0 ) {
	      fprintf(stderr, "getsockname() failed to get port number\n");
	      exit(4);
	    }
	    //fprintf(stderr,"The assigned port is %d\n", ntohs(sin.sin_port));
	    assigned_port = ntohs(sin.sin_port);
	  }

    }

    //IFVERB printf("((((())))) got socket with handle %d (%d)\n", sd, errno);
    //myx = eof(sd);
    //pIFVERB rintf("((((())))) checked socket: %d (%d)\n", myx, errno);

    if (p_assigned_port!=NULL)
      {
	*p_assigned_port = assigned_port;
      }

    //SetOptions(sd);
  
    return sd;
}

int sockinfo(int sd, char *info)
{
    struct sockaddr_in sin;
    size_t len = sizeof(sin);
    memset(&sin, 0, sizeof(sin));
    if (getsockname(sd, (struct sockaddr *) &sin, &len) == -1) {
		sockerror("sockinfo");
		return -1;
	}
    memcpy(info, &sin.sin_addr, 4);
    memcpy(info + 4, &sin.sin_port, 2);
    return 0;
}

int sockclose(int sock)
{
#ifdef __WIN_MSVC__
	return closesocket(sock);
#else
    return close(sock);
#endif
}

#ifdef __WIN_MSVC__

typedef struct {
	void (*handler)(void *);
	int socket;
} socketHandlerInfo;

void handlerThread(void *param) {
	socketHandlerInfo *info = (socketHandlerInfo *)param;
	info->handler((void *)info->socket);
	sockclose(info->socket);
	free(info);
	/* implicit exit here */
}
#endif

void starthandler(void (*handler) (void *), int socket) 
{
#ifndef SERIALIZE_LISTEN
#ifdef __WIN_MSVC__
  socketHandlerInfo *info = (socketHandlerInfo *)malloc(sizeof(socketHandlerInfo));
  info->handler = handler;
  info->socket = socket;
  _beginthread(handlerThread, 0, (void *)info);
#else
  pid_t pid;
  pid = fork();
  if (pid == 0) {
    handler((void *)socket);
    exit(0);
  } else if (pid == -1)
    perror("fork");
  else
    close(socket);
  
#endif
#else
    handler((void *)socket);
    close(socket);
#endif
}

void socklisten(int sock, void (*handler) (void *))
{
    struct sockaddr sockaddr;
    size_t addrlen = sizeof(sockaddr);
    int newsock;

   	if (listen(sock, 5) == -1) {
		sockerror("socklisten(listen)");
		exit(0);
	}
    while (1) {
		memset(&sockaddr, 0, sizeof(sockaddr));
		newsock = accept(sock, &sockaddr, &addrlen);
        if (newsock != -1) {
			starthandler(handler, newsock);
        } else {
			sockerror("socklisten(accept)");
			exit(0);
		}
    }
}

/*
 * senddata 
 *    - appends <CR><LF> before transmitting data through socket.
 *    - saves intermediate copy by using writev
 *    - is O(n) where n is sizeo of stream
 *    - guarantees atomic write of two-part data
 * bugs:
 *    - __WIN_MSVC__ implementation should be rewritten using WSASend
 */
int senddata(int socket, char *buffer, int length)
{
#ifndef __WIN_MSVC__
    static char crlf[] = "\r\n";
    static struct iovec iov[] =
    {
        {NULL, 0},
        {crlf, 2}};
    iov[0].iov_base = buffer;
    iov[0].iov_len = length;
    length = writev(socket, iov, 2);
    if (length >= 0)
        length -= 2;
	return length;
#endif
#ifdef __WIN_MSVC__
	char *buffer2 = (char *)malloc(length+2);
	memcpy(buffer2, buffer, length);
	memcpy(buffer2+length, "\r\n", 2);
	length = send(socket, buffer2, length+2, 0);
	if (length > 0)
		length -= 2;
	free(buffer2);
	return length;
#endif
}

/*
 * recvdata
 *    - extracts <cr><lf>-delimited record from socket stream
 *    - works with fragmented socket stream
 *    - fragments returned records correctly if sent size > maxsize
 *    - works correctly with any size recvbuffer
 *    - is O(n) where n = sizeof stream
 * bugs:
 *    - uses static state information and hence must always be
 *      called with the same socket. If called with two different sockets,
 *      call with second socket may return data left over from call with
 *      first socket.
 */
int recvdata(int socket, char *buffer, int maxsize)
{
    static char recvbuffer[1024];
    static char *bufp = recvbuffer,
     *bufe = recvbuffer;
    char *dst = buffer;
    static enum {
        stData, stCr, stCopy
    } state = stData;
    while (dst - buffer < maxsize) {
        if (bufp == bufe) {
            int length = recv(socket, recvbuffer, sizeof(recvbuffer), 0);
            if (length < 0)
                if (dst - buffer > 0)
                    return dst - buffer;
                else
                    return length;
            bufp = recvbuffer;
            bufe = recvbuffer + length;
            if (length == 0) {
				*dst= 0;
                break;
			}
        }
        switch (state) {
            case stData:
                if (*bufp == '\r') {
                    state = stCr;
                    bufp++;
                } else
                    *dst++ = *bufp++;
                break;
            case stCr:
                if (*bufp == '\n') {
                    state = stData;
                    bufp++;
                    return dst - buffer;
                } else {
                    state = stCopy;
                    *dst++ = '\r';
                }
                break;
            case stCopy:
                *dst = *bufp++;
                state = stData;
                break;
        }
    }
	buffer[maxsize-1] = '\0';
    return maxsize;
}

/*
 * recvstring
 *    - like recvdata but without the bugs.
 *    - requires an initialized recvbuffer_t.
 *    - extracts <cr><lf>-delimited record from socket stream
 *    - works with fragmented socket stream
 *    - fragments returned records correctly if sent size > maxsize
 *    - works correctly with any size recvbuffer
 *    - is O(n) where n = sizeof stream
 */

recvbuffer_t *recvBufferCreate(int size) {
	recvbuffer_t *rb = (recvbuffer_t *) malloc (sizeof(recvbuffer_t));
	rb->recvbuffer = (char *)malloc(size);
	rb->bufp = rb->recvbuffer;
	rb->bufe = rb->recvbuffer;
	rb->size = size;
	return rb;
}

void recvBufferDestroy(recvbuffer_t *rb) {
	free(rb->recvbuffer);
	free(rb);
}

int recvstring(int socket, char *buffer, int maxsize, recvbuffer_t *rb)
{
    char *dst = buffer;
    static enum {
        stData, stCr, stCopy
    } state = stData;
    while (dst - buffer < maxsize -	1) {
        if (rb->bufp == rb->bufe) {
            int length = recv(socket, rb->recvbuffer, rb->size, 0);
            if (length < 0)
                if (dst - buffer > 0) {
					*dst = '\0';
                    return dst - buffer;
                } else {
					buffer[0] = '\0';
                    return length;
				}
            rb->bufp = rb->recvbuffer;
            rb->bufe = rb->recvbuffer + length;
            if (length == 0) {
				*dst = '\0';
                break;
			}
        }
        switch (state) {
            case stData:
                if (*rb->bufp == '\r') {
                    state = stCr;
                    rb->bufp++;
                } else
                    *dst++ = *rb->bufp++;
                break;
            case stCr:
                if (*rb->bufp == '\n') {
                    state = stData;
                    rb->bufp++;
					*dst = '\0';
                    return dst - buffer;
                } else {
                    state = stCopy;
                    *dst++ = '\r';
                }
                break;
            case stCopy:
                *dst = *rb->bufp++;
                state = stData;
                break;
        }
    }
	buffer[maxsize-1] = '\0';
    return maxsize;
}



int sendstring(int socket, char *string) {
	return senddata(socket, string, strlen(string));
}
