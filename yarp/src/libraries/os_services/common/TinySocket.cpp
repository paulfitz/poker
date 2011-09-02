
#include <stdio.h>

#include <assert.h>

#include <sys/types.h>
#ifdef __WIN_MSVC__
#include <winsock2.h>
#define ioctl ioctlsocket
#include <io.h>
#ifdef Yield
#undef Yield
#endif
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

/*
#ifdef WIN32
#include <winsock.h>
#include <process.h>
#include <io.h>
#include <stdio.h>
#else
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#endif

#ifdef WIN32
#include <winsock.h>
#include <stdio.h>
#include <errno.h>
*/

extern "C" {
#include "socklib.h"
}

#include <stdlib.h>
#include <errno.h>

#include <list>

#ifndef __QNX__
using namespace std;
#endif

#include "YARPThread.h"
#include "YARPSemaphore.h"
#include "YARPTime.h"
#include "TinySocket.h"
//#include "Semaphore.h"
#include "YARPNetworkTypes.h"

#include "YARPScheduler.h"

int TINY_VERBOSE = 0;
#define IFVERB if(TINY_VERBOSE) 

//#ifndef __QNX4__
//#define USE_PACKING __attribute__ ((packed))
//#endif

#ifdef __QNX__
#define MSG_NOSIGNAL (0x2000)
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

/*
#include <signal.h>

static void SigHandler(int sig)
{
  printf
}
*/


#include "begin_pack_for_net.h"
struct MyMessageHeader
{
public:
  char key[2];
  NetInt32 len;
  char key2[2];

  MyMessageHeader()
    {
      len = 0;
      SetBad();
      IFVERB printf(">>>> header created with size %d\n", 
		    sizeof(MyMessageHeader));
      IFVERB printf(">>>> len part has size %d\n", 
		    sizeof(NetInt32));
      IFVERB printf(">>>> one char has size %d\n", 
		    sizeof(char));
      //      IFVERB printf(">>>> i count %d + %d + %d\n", 
      //	    sizeof(key),sizeof(len),sizeof(key2));
    }

  void SetGood()
    {
      key[0] = 'Y';
      key[1] = 'A';
      key2[0] = 'R';
      key2[1] = 'P';
    }
  
  void SetBad()
    {
      key[0] = 'y';
      key[1] = 'a';
      key2[0] = 'r';
      key2[1] = 'p';
    }

  void SetLength(int n_len)
    {
      len = n_len;
    }
  
  int GetLength()
    {
      if (key[0]=='Y'&&key[1]=='A'&&
	  key2[0]=='R'&&key2[1]=='P')
	{
	  return len;
	}
      else
	{
	  //	  printf("*** Bad header\n");
	  return -1;
	}
    }
} PACKED_FOR_NET;
#include "end_pack_for_net.h"


static void StartSockets()
{
#ifdef __WIN_MSVC__
	static int started = 0;
	if (!started) {
		IFVERB printf("[[[]]] starting up winsock\n");
		short wVersionRequested = MAKEWORD( 2, 2 );
		WSADATA wsaData;
		if (WSAStartup( wVersionRequested, &wsaData ) == -1) {
			sockerror("sockopen");
			exit(0);
		}
		/*
		if (wsaData.wVersion != 0x101) {
			fprintf(stderr, "Incorrect winsock version\n");
			exit(0);
		}
		*/
		started = 1;
	}
#endif
};

class AutoStartSockets
{
public:
	AutoStartSockets() { StartSockets(); }
} single_instance;


int TinySocket::IsData()
{ 
#ifdef __WIN_MSVC__
  unsigned long int len = 0;
#else
  int len = 0;
#endif
  //Check();   //PFHIT
  if (pid>0) ioctl(pid,FIONREAD,&len);
  return len;
}

int TinySocket::WaitForData()
{ 
  int len = 0;
  char buf[1];
  if (pid>0)
    {
      Read(buf,0,0);
    }
  return len;
}

void TinySocket::Verbose(int flag)
{
  TINY_VERBOSE = flag;
}

int TinySocket::MakeClient(const char *host,int port)
{
  IFVERB printf("Host is %s and port is %d\n", host, port);
  int sock = sockopen((char *)host, port,NULL);
  //char buffer[256];
  unsigned char info[6];
  recvbuffer_t *prb = recvBufferCreate(1024);;
  if (sock == -1) {
    sockerror("client.sockopen");
    pid = -1;
    return -1;
    //exit(1);
  }
  sockinfo(sock, (char *) info);
  IFVERB printf("Client socket open on host %d.%d.%d.%d, port %d\n",
		info[0], info[1], info[2], info[3], info[4] * 256 + info[5]);

  /*
  while (gets(buffer) != NULL) {
    sendstring(sock, buffer);
    recvstring(sock, buffer, sizeof(buffer), prb);
    puts(buffer);
  }
  sockclose(sock);
  */
  pid = sock;
  return sock;
}

void foo(int num)
{
  printf("PIPE!!\n");
}

int TinySocket::Write(const char *buf, int len)
{
  int result = -1;
  if (pid>0)
    {
      IFVERB printf("HEY! Writing %d bytes to %d\n",len,pid);
      /*
      //signal( SIGCHLD, SIG_IGN );  
      signal( SIGPIPE, foo );
      result = write(pid,buf,len);
      //result = send(pid,buf,len,0);
       */
      result = send(pid,buf,len,MSG_NOSIGNAL);
      //result = write(pid,buf,len); //won't work in windows
      IFVERB printf("Wrote (%d)\n", result);
    }
  return result;
}

int TinySocket::Read(char *buf, int len, int exact)
{
  int sofar = 0;
  int nReadAmount = 0;
  if (len>0)
    {
      if (pid>0)
	{
	  IFVERB printf("Reading for pid %d\n", pid);
	  do {
	    do {
	      nReadAmount=recv(pid,buf+sofar,len-sofar,MSG_NOSIGNAL);
	      IFVERB printf("Read loop %d: %d\n", pid, nReadAmount);
	      if (nReadAmount<0)
		{
		  IFVERB printf("~~~~~~~~~~~ errno %d -- %d\n", errno, pid);
		}
	      if (nReadAmount==0 && len>0) { nReadAmount=-1; }
	      if (nReadAmount>0)
		{
		  sofar += nReadAmount;
		}
	    } while (nReadAmount>0 && sofar<len);
	  } while (sofar<len && exact && nReadAmount>=0);
	  IFVERB printf("Got %d bytes of %d\n", sofar, len);
	}
    }
  else
    {
      IFVERB printf("Got %d bytes of %d\n", sofar, len);
    }
  return (sofar>0)?sofar:nReadAmount;
}

int TinySocket::Close()
{
  if (pid>0)
    {
      sockclose(pid);
    }
  pid = 0;
  return pid;
}


class SocketThread : public YARPThread
{
public:
  TinySocket socket;
  int available;
  SocketOwner *owner;
  char *extern_buffer;
  int extern_length;
  char *extern_reply_buffer;
  int extern_reply_length;
  int waiting;
  int needs_reply;
  int read_more;
  int reply_preamble;
  YARPSemaphore wakeup, mutex, reply_made;

  SocketThread(int n_pid = -1) : wakeup(0), mutex(1), reply_made(0)
    {
      owner = NULL;
      extern_buffer = NULL;
      extern_length = 0;
      extern_reply_buffer = NULL;
      extern_reply_length = 0;
      waiting = 0;
      needs_reply = 0;
      if (n_pid!=-1) SetPID(n_pid);
      available = 1;
      read_more = 0;
      reply_preamble = 0;
    }

  /*
  // hack to deal with stl unpleasantness
  SocketThread(const SocketThread& other)
    {
      owner = NULL;
      extern_buffer = NULL;
      extern_length = 0;
      extern_reply_buffer = NULL;
      extern_reply_length = 0;
      waiting = 0;
      needs_reply = 0;      
      available = 1;
      read_more = 0;
      reply_preamble = 0;
    }
  */

  int operator == (const SocketThread& other) { return 0; }
  int operator != (const SocketThread& other) { return 0; }
  int operator <  (const SocketThread& other) { return 0; }

  void SetOwner(SocketOwner& n_owner)
    {
      owner = &n_owner;
    }

  void SetPID (int n_pid)
    {
      IFVERB printf("$$$ changing from %d to %d\n", 
		    socket.GetSocketPID(), n_pid);
      socket.ForcePID(n_pid);
      //owner = NULL;
      extern_buffer = NULL;
      extern_length = 0;
      waiting = 0;
      needs_reply = 0;
      read_more = 0;
      reply_preamble = 0;
    }

  int GetPID()
    {
      return  socket.GetSocketPID();
    }

  virtual void Body()
    {
      /*
      while (1)
	{
      */
      int finished = 0;
      //owner = NULL;
      extern_buffer = NULL;
      extern_length = 0;
      waiting = 0;
      needs_reply = 0;
      read_more = 0;
      reply_preamble = 0;
      while (!finished)
	{
	  IFVERB printf("*** listener %d waiting\n", socket.GetSocketPID());
	  //char buf[1000];
	  MyMessageHeader hdr;
	  hdr.SetBad();
	  int r = -1;
	  r = socket.Read((char*)(&hdr),sizeof(hdr),1);
	  if (r<0)
	    {
	      IFVERB printf("*** closing\n", r);
	      socket.Close();
	      finished = 1;
	    }
	  int len = hdr.GetLength();
	  if (len<0)
	    {
	      IFVERB printf("{{}} Corrupt/empty header received\n");
	    }
	  if (len>=0)
	    {
	      IFVERB printf ("*** got a header\n");
	      if (owner!=NULL)
		{
		  IFVERB printf ("*** and i am owned\n");
		  mutex.Wait();
		  extern_buffer = NULL;
		  extern_length = len;
		  owner->DeclareDataAvailable();
		  waiting = 1;
		  mutex.Post();
		  wakeup.Wait();
		  IFVERB printf ("*** woken up!\n");
		}

	      if (extern_buffer!=NULL)
		{
		  /*
		  if (len>extern_length)
		    {
		      printf("message is too big (%d/%d)\n", len, extern_length);
		    }
		  assert(len<=extern_length);
		  */

		  if (len>extern_length)
		    {
		      len = extern_length;
		    }
		  r = socket.Read(extern_buffer,len,1);
		  extern_length = r;
		  int rep = needs_reply;
		  owner->DeclareDataWritten();
		  if (rep)
		    {
		      wakeup.Wait();
		      needs_reply = 0;
		    }
		  while (read_more)
		    {
		      int r = socket.Read(extern_reply_buffer,
					  extern_reply_length,1);
		      extern_reply_length = r;
		      read_more = 0;
		      reply_made.Post();
		      wakeup.Wait();
		      needs_reply = 0;
		    }
		  int was_preamble = 0;
		  do
		    {
		      if (reply_preamble)
			{
			  rep = 1;
			}
		      MyMessageHeader hdr2;
		      hdr2.SetGood();
		      char bufack[] = "acknowledged";
		      char *buf3 = bufack;
		      int reply_len = 0; //strlen(bufack)+1;
		      if (rep)
			{
			  buf3 = extern_reply_buffer;
			  reply_len = extern_reply_length;
			}
		      hdr2.SetLength(reply_len);
		      socket.Write((char*)(&hdr2),sizeof(hdr2));
		      if (reply_len>0)
			{
			  socket.Write(buf3,reply_len);
			}
		      int curr_preamble = reply_preamble;
		      if (rep)
			{
			  IFVERB printf("*** POSTING reply made %d\n", 
					curr_preamble);
			  reply_made.Post();
			}
		      if (r>=0)
			{
			  IFVERB printf("*** listener got %d bytes\n", 
					r);
			}
		      if (r<0)
			{
			  IFVERB printf("*** closing\n", r);
			  socket.Close();
			  finished = 1;
			}
		      was_preamble = 0;
		      if (curr_preamble)
			{
			  was_preamble = 1;
			  
			  IFVERB printf("*** WAITING for post-preamble wakeup\n", r);
			  wakeup.Wait();
			  IFVERB printf("*** DONE WAITING for post-preamble wakeup\n", r);
			  rep = 1;
			}
		    }
		  while (was_preamble);
		}
	    }
	  //YARPTime::DelayInSeconds(1);
	}

      available = 1;
      /*
      printf("--------- read thread going down...\n");
      wakeup.Wait();
      printf("--------- read thread coming back up for reuse...\n");
	}
      */

#ifdef WIN32
	  IFVERB printf("*** comms thread bailed out\n");
#else
      IFVERB printf("*** comms thread %d bailed out\n", getpid());
#endif
    }
    

  void SetAvailable(int flag)
    {
      available = flag;
    }
};


class MySocketInfo
{
public:
  struct sockaddr sockaddr;
  list<SocketThread *> slave;

  YARPSemaphore new_data, new_data_written;

  MySocketInfo() : new_data(0), new_data_written(0) {}
  
  virtual ~MySocketInfo()
    {
      for (list<SocketThread*>::iterator it = slave.begin(); it!=slave.end(); it++)
	{
	  delete (*it);
	}
    }
};

SocketOwner::SocketOwner(const SocketOwner& other)
{
  printf("Copying socket owner\n");
  assert(other.system_resources == NULL);
  pid = -1;  system_resources = NULL;
}


SocketOwner::~SocketOwner()
{
  if (system_resources!=NULL)
    {
      delete ((MySocketInfo*) system_resources);
      system_resources = NULL;
    }
}

int SocketOwner::Close()
{
  if (system_resources!=NULL)
    {
      MySocketInfo& info = *((MySocketInfo*)system_resources);
      list<SocketThread*>::iterator it_avail;
      for (it_avail=info.slave.begin(); it_avail!=info.slave.end(); it_avail++)
	{
	  (*it_avail)->End();
	}
    }
  End();
  if (pid>0)
    {
      sockclose(pid);
    }
  pid = 0;
  return 1;
}

int SocketOwner::Close(int reply_id)
{
  int result = -1;
  list<SocketThread*>::iterator it_avail;
  assert(system_resources!=NULL);
  MySocketInfo& info = *((MySocketInfo*)system_resources);
  for (it_avail=info.slave.begin(); it_avail!=info.slave.end(); it_avail++)
    {
      if (!((*it_avail)->available))
	{
	  if ((*it_avail)->GetPID()==reply_id)
	    {
	      (*it_avail)->End();
	      (*it_avail)->available = 1;
	    }
	}
    }
  if (reply_id!=-1)
    {
      sockclose(reply_id);
      IFVERB printf("!!!!!!!!!!!!! closed handle %d\n", reply_id);
    }
  return result;
}

int SocketOwner::Connect(int port)
{
  //printf("&&& Looking to open port\n");
  int new_assigned_port = port;
  int sock = sockopen(NULL, port,&new_assigned_port);
  assigned_port = new_assigned_port;
  unsigned char info[6];
  Close();
  if (sock == -1) {
    perror("server.sockopen");
    //exit(1);
    pid = sock;
  }
  else
    {
      sockinfo(sock, (char *)info);
      IFVERB printf("Server socket open on host %d.%d.%d.%d, port %d\n",
		    info[0], info[1], info[2], info[3], info[4] * 256 + info[5]);
      
      pid = sock;
      //printf("System res are %ld\n", system_resources);
      if (system_resources==NULL)
	{
	  //printf("Check1\n");
	  fflush(stdout);
	  system_resources = new MySocketInfo;
	  //printf("Check2\n");
	  fflush(stdout);
	  assert(system_resources!=NULL);
	}
      /*
	socklisten(sock, handler);
      */
      //printf("Check3\n");
      fflush(stdout);
      if (listen(sock, 5) == -1) {
	sockerror("socklisten(listen)");
	exit(0);
      }

      //printf("About to begin\n");
      Begin();
      //printf("Begun\n");
    }

  IFVERB printf("Server socket is %d\n", sock);
  return sock;
}

void SocketOwner::DeclareDataAvailable()
{
  assert(system_resources!=NULL);
  MySocketInfo& info = *((MySocketInfo*)system_resources);
  IFVERB printf("$$$ Declaring data available\n");
  info.new_data.Post();
}

void SocketOwner::DeclareDataWritten()
{
  assert(system_resources!=NULL);
  MySocketInfo& info = *((MySocketInfo*)system_resources);
  IFVERB printf("$$$ Declaring data written\n");
  info.new_data_written.Post();
}

void SocketOwner::AddSocket()
{
  // need to keep calling this to get next incoming socket

#ifdef __QNX__
  signal( SIGCHLD, SIG_IGN );     /* Ignore condition */
#endif

  struct sockaddr sockaddr;
#ifndef __WIN__
#ifndef __QNX4__
  unsigned int addrlen = sizeof(sockaddr);
#else
  int addrlen = sizeof(sockaddr);
#endif
#else
  int addrlen = sizeof(sockaddr);
#endif 
  int newsock;
  

  assert(pid!=-1);
  memset(&sockaddr, 0, sizeof(sockaddr));
  YARPScheduler::Yield();
  IFVERB printf("888888 pre accept %d\n", errno);
  newsock = accept(pid, &sockaddr, &addrlen);
  IFVERB printf(">>> accepting a new socket %d (from in %d)\n", 
		newsock, pid);
  IFVERB printf("888888 post accept %d\n", errno);

  /*
  static int count = 0;
  count ++;
  if (count>=0)
    {
      printf("======================================================\n");
      printf("RUNNING TESTS\n");
      TinySocket is;
      is.ForcePID(newsock);
      char buf[256];
      is.Read(buf,100);
      printf("======================================================\n");
    }
*/
  
  //YARPScheduler::Yield();
  assert(system_resources!=NULL);
  if (newsock != -1) {
    MySocketInfo& info = *((MySocketInfo*)system_resources);
    list<SocketThread*>::iterator it_avail;
    int reusing = 0;
    for (it_avail=info.slave.begin(); it_avail!=info.slave.end(); it_avail++)
      {
	if ((*it_avail)->available)
	  {
	    reusing = 1;
	    break;
	  }
      }
    IFVERB printf("888888 pre pushback %d\n", errno);
    if (it_avail==info.slave.end())
      {
	info.slave.push_back(new SocketThread);
	it_avail = info.slave.end();
	it_avail--;
      }
    IFVERB printf("888888 post pushback %d\n", errno);
    (*it_avail)->SetAvailable(0);
    (*it_avail)->SetOwner(*this);
    (*it_avail)->SetPID(newsock);
    IFVERB printf("888888 pre postbegin %d\n", errno);
    if (!reusing)
      {
	(*it_avail)->Begin();
      }
    else
      {
	(*it_avail)->End();
	(*it_avail)->Begin();
	//(*it_avail).wakeup.Post();
      }
    IFVERB printf("888888 post postbegin %d\n", errno);
    for (list<SocketThread*>::iterator it=info.slave.begin(); 
	 it!=info.slave.end(); it++)
      {
	IFVERB printf("   pid %d...\n", (*it)->GetPID());
	//int pre = errno;
	//int v = eof((*it).GetPID());
	//int post = errno;
	//IFVERB printf("   --> %d %d %d\n", pre, v, post);
      }
    //close(newsock);
    //starthandler(handler, newsock);
  } else {
    sockerror("socklisten(accept)");
    exit(0);
  }
}

int SocketOwner::Read(char *buf, int len, int *reply_pid)
{
  int save_pid = -1;

#ifdef __QNX__
  signal( SIGCHLD, SIG_IGN );     /* Ignore condition */
#endif
  
  assert(system_resources!=NULL);
  MySocketInfo& info = *((MySocketInfo*)system_resources);

  int finished = 0;
  int result = -1;
  while (!finished)
    {
      IFVERB printf("### Waiting for new data\n");
      info.new_data.Wait();
      IFVERB printf("### Got new data\n");
      list<SocketThread *>::iterator it_avail;
      for (it_avail=info.slave.begin(); it_avail!=info.slave.end(); it_avail++)
	{
	  if (!((*it_avail)->available))
	    {
	      int work = 0, in_len = 0;
	      (*it_avail)->mutex.Wait();
	      if ((*it_avail)->waiting)
		{
		  IFVERB printf("### Identified source of new data\n");
		  work = 1;
		  (*it_avail)->waiting = 0;
		  (*it_avail)->extern_buffer = buf;
		  in_len = (*it_avail)->extern_length;
		  (*it_avail)->extern_length = len;
		  if (reply_pid!=NULL)
		    {
		      (*it_avail)->needs_reply = 1;
		    }
		  else
		    {
		      (*it_avail)->needs_reply = 1;
		    }
		  (*it_avail)->wakeup.Post();
		  IFVERB printf("### Waking up source of new data\n");
		}
	      (*it_avail)->mutex.Post();
	      if (work)
		{
		  IFVERB printf("### Waiting for buffer write\n");
		  info.new_data_written.Wait();
		  save_pid = (*it_avail)->GetPID();
		  IFVERB printf ("@@@ got data %d/%d\n", 
				 in_len, len);
		  result = in_len;
		  finished = -1;
		  break;
		}
	    }
	}
    }

  if (reply_pid!=NULL)
    {
      *reply_pid = save_pid;
    }

  return result;
}

int SocketOwner::PollingRead(char *buf, int len, int *reply_pid)
{
  assert(system_resources!=NULL);
  MySocketInfo& info = *((MySocketInfo*)system_resources);

  if (reply_pid!=NULL)
    {
      *reply_pid = 0;
    }
  
  list<SocketThread *>::iterator it_avail;
  for (it_avail=info.slave.begin(); it_avail!=info.slave.end(); it_avail++)
    {
      if (!((*it_avail)->available))
	{
	  if ((*it_avail)->waiting)
	    {
	      return Read(buf,len,reply_pid);
	    }
	}
    }
  return 0;
}


int SocketOwner::BeginReply(int reply_pid, char *buf, int len)
{
  assert(system_resources!=NULL);
  MySocketInfo& info = *((MySocketInfo*)system_resources);

  IFVERB printf("&&& BEGINNING REPLY of %d bytes\n", len);

  list<SocketThread *>::iterator it_avail;
  for (it_avail=info.slave.begin(); it_avail!=info.slave.end(); it_avail++)
    {
      if (!((*it_avail)->available))
	{
	  if ((*it_avail)->GetPID() == reply_pid)
	    {
	      (*it_avail)->mutex.Wait();
	      (*it_avail)->extern_reply_buffer = buf;
	      (*it_avail)->extern_reply_length = len;
	      (*it_avail)->read_more = 0;
	      (*it_avail)->needs_reply = 1;
	      (*it_avail)->reply_preamble = 1;
	      (*it_avail)->wakeup.Post();
	      (*it_avail)->mutex.Post();
	      (*it_avail)->reply_made.Wait();
	    }
	}
    }

  IFVERB printf("&&& FINISHED BEGINNING REPLY of %d bytes\n", len);

  return 0;
}

int SocketOwner::Reply(int reply_pid, char *buf, int len)
{
  assert(system_resources!=NULL);
  MySocketInfo& info = *((MySocketInfo*)system_resources);

  IFVERB printf("&&& BEGINNING FINAL REPLY of %d bytes\n", len);

  list<SocketThread *>::iterator it_avail;
  for (it_avail=info.slave.begin(); it_avail!=info.slave.end(); it_avail++)
    {
      if (!((*it_avail)->available))
	{
	  if ((*it_avail)->GetPID() == reply_pid)
	    {
	      (*it_avail)->mutex.Wait();
	      (*it_avail)->extern_reply_buffer = buf;
	      (*it_avail)->extern_reply_length = len;
	      (*it_avail)->read_more = 0;
	      (*it_avail)->needs_reply = 1;
	      (*it_avail)->reply_preamble = 0;
	      (*it_avail)->wakeup.Post();
	      (*it_avail)->mutex.Post();
	      (*it_avail)->reply_made.Wait();
	    }
	}
    }

  IFVERB printf("&&& FINISHED BEGINNING FINAL REPLY of %d bytes\n", len);

  return 0;
}

int SocketOwner::ReceiveMore(int reply_pid, char *buf, int len)
{
  int result = -1;
  assert(system_resources!=NULL);
  MySocketInfo& info = *((MySocketInfo*)system_resources);

  list<SocketThread *>::iterator it_avail;
  for (it_avail=info.slave.begin(); it_avail!=info.slave.end(); it_avail++)
    {
      if (!((*it_avail)->available))
	{
	  if ((*it_avail)->GetPID() == reply_pid)
	    {
	      (*it_avail)->mutex.Wait();
	      (*it_avail)->extern_reply_buffer = buf;
	      (*it_avail)->extern_reply_length = len;
	      (*it_avail)->read_more = 1;
	      (*it_avail)->wakeup.Post();
	      (*it_avail)->mutex.Post();
	      (*it_avail)->reply_made.Wait();
	      result = (*it_avail)->extern_reply_length;
	    }
	}
    }
  return result;
}


int Receive(SocketOwner& owner, char *buf, int len, int *reply_pid)
{
  return owner.Read(buf,len,reply_pid);
}

int Reply(SocketOwner& owner, int pid, char *buf, int len)
{
  return owner.Reply(pid,buf,len);
}

int ReceiveMore(SocketOwner& owner, int pid, char *buf, int len)
{
  return owner.ReceiveMore(pid,buf,len);
}

int Send(TinySocket& c, char *msg, int msg_len, char *in_msg, int in_msg_len)
{
  int result = -1;
  MyMessageHeader hdr, hdr2;
  hdr.SetGood();
  hdr.SetLength(msg_len);
  c.Write((char *)(&hdr),sizeof(hdr));
  c.Write(msg,msg_len);
  hdr2.SetBad();
  int r = c.Read((char *)(&hdr2),sizeof(hdr2),1);
  if (r==sizeof(hdr2))
    {
      int len2 = hdr2.GetLength();
      if (len2>0)
	{
	  if (len2<in_msg_len)
	    {
	      in_msg_len = len2;
	    }
	  result = c.Read(in_msg,in_msg_len,1);
	}
      else
	{
	  if (len2==0) { result = 0; }
	}
    }
  return result;
}

int SendBlock(TinySocket& c, char *msg, int msg_len, int header)
{
  int result = -1;
  MyMessageHeader hdr;
  if (header)
    {
      hdr.SetGood();
      hdr.SetLength(msg_len);
      c.Write((char *)(&hdr),sizeof(hdr));
    }
  c.Write(msg,msg_len);
  return 0;
}

int SendBeginEnd(TinySocket& c, char *in_msg, int in_msg_len)
{
  return SendEnd(c,in_msg,in_msg_len);
}


int SendEnd(TinySocket& c, char *in_msg, int in_msg_len)
{
  int result = -1;
  MyMessageHeader hdr2;
  hdr2.SetBad();
  int r = c.Read((char *)(&hdr2),sizeof(hdr2),1);
  if (r==sizeof(hdr2))
    {
      int len2 = hdr2.GetLength();
      if (len2>0)
	{
	  if (len2<in_msg_len)
	    {
	      in_msg_len = len2;
	    }
	  result = c.Read(in_msg,in_msg_len,1);
	}
      else
	{
	  if (len2==0) { result = 0; }
	}
    }
  return result;
}

/*
void server(int port)
{
  SocketOwner s;
  s.Connect(port);
  while (1)
    {
      char buf[1000] = "not set";
      int reply_id = -1;
      int len = Receive(s,buf,sizeof(buf),&reply_id);
      printf("Get data (%d): %s\n", len, buf);
      char reply[1000];
      sprintf(reply,"I acknowledge --> %s", buf);
      Reply(s,reply_id,reply,strlen(reply)+1);
    }
}
*/

/*
void client(char *serverhost, int port)
{
  TinySocket c;
  c.Connect(serverhost,port);
  char buf[1000];
  char buf2[1000];

  for (int i=0; i<20; i++)
    {
      sprintf(buf, "hello there time #%d", i);
      int len = strlen(buf)+1;
      //int len2 = Send(c,buf,len,buf2,sizeof(buf2));
      SendBlock(c,buf,len);
      int len2 = SendEnd(c,buf2,sizeof(buf2));
      if (len2>0)
	{
	  printf("Got response (%d): %s\n", len2, buf2);
	}
      else
	{
	  printf("Got response (%d)\n", len2);
	}
      YARPTime::DelayInSeconds(2.0);
    }
}
*/

/*

int main(int argc, char *argv[])
{
  //    TinySocket::Verbose();

    if (argc == 3)
      {
        client(argv[1], atoi(argv[2]));
      }
    else if (argc == 2)
      {
        server(atoi(argv[1]));
      }
    return 0;
}

*/


#ifndef __QNX__
string GetBase(const char *name)
{
  string work = name;
  int pos = work.find("|",0);
  if (pos != string::npos)
    {
      if (pos>0)
	{
	  work = work.substr(0,pos);
	}
      else
	{
	  work = "";
	}
    }
  else
    {
      work = "";
    }
  return work;
}

int GetPort(const char *name)
{
  string work = name;
  int pos = work.find("|",0);
  int len = work.size()-pos-1;
  if (pos != string::npos)
    {
      work = work.substr(pos+1,len);
    }
  //DBG printf("Port part is from %d, length %d (%s)\n", 
  //pos, len, work.c_str());
  return atoi(work.c_str());
}

#else

#include <string.h>

string GetBase(const char *name)
{
  string work = name;
  string work2 = "";
  char *pos = (char*)strchr(work.AsChars(),'|');
  if (pos != NULL)
    {
      *pos = '\0';
      work2 = work;
      *pos = '|';
    }
  return work2;
}

int GetPort(const char *name)
{
  string work = name;
  string work2 = "";
  char *pos = (char*)strchr(work.AsChars(),'|');
  if (pos != NULL)
    {
      return(atoi(pos+1));
    }
  return 0;
}

#endif
