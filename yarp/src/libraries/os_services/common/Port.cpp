
//#include "debug-new.h"

#include <stdio.h>
//#include <sys/kernel.h>
#include <assert.h>
//#include <semaphore.h>
#include <stdarg.h>
//#include <sys/name.h>
//#include <sys/sendmx.h>
//#include <sys/types.h>

#include "YARPThread.h"
#include "mesh.h"

#include "YARPString.h"

#include "BlockSender.h"
#include "BlockReceiver.h"

#include "Port.h"

#include "debug.h"
#include "RefCounted.h"

#include "YARPNameService.h"
#include "YARPSyncComm.h"
#include "YARPScheduler.h"
#include "YARPTime.h"

Sema services_sema(1);


void safe_printf(char *format,...)
{
  va_list arglist;
  services_sema.Wait();
  va_start(arglist,format);
  vprintf(format,arglist);
  va_end(arglist);
  services_sema.Post();
}


#define MAX_PACKET (128*128*3+100)
//#define MAX_PACKET (150)
#define INT_SIZE (sizeof(int))
#define MXFER_SIZE (sizeof(struct _mxfer_entry))
#define HEADER_CT (4)

#define MAX_FRAGMENT (4)

/*
class FragmentHeader
{
public:
  char tag;
  char first;
  char more;
  int blocks;
  int first_offset;
  int final_offset;
  int lengths[MAX_FRAGMENT+1];

  int top;
  char reply_buf[10];
  struct _mxfer_entry mx_reply;

  struct _mxfer_entry mx0;
  struct _mxfer_entry mx[MAX_FRAGMENT+1];

  FragmentHeader()
    {
      _setmx(&mx_reply,&reply_buf,sizeof(reply_buf));
      top = 0;
    }

  void Clear()
    {
      top = 0;
      first_offset = final_offset = 0;
    }

  void Add(char *buf, int len, int lo, int hi)
    {
      _setmx(&mx[top],buf+lo,hi-lo);
      lengths[top] = len;
      if (top==0) first_offset = lo;
      if (hi<len) final_offset = hi; else final_offset = 0;
      top++;
    }

  void Complete(int nfirst, int nmore)
    {
      tag = MSG_ID_NULL;
      blocks = top;
      more = nmore;
      first = nfirst;
      _setmx(&mx0,(void*)&tag,GetHeaderSize(top));
    }

  int Fire(int pid)
    {
      int success = 1;
      int failrepeat = 2;
      int repeat = 1;
      while (repeat)
	{
	  dbg_printf(95)("Sending component\n");
	  if (Sendmx(pid,top+1,1,&mx0,&mx_reply)==-1)
	    {
	      if (failrepeat>0) 
		{
		  failrepeat--;
		  dbg_printf(95)("Error sending fragment... resend\n");
		  repeat = 1;
		}
	      else
		{
		  dbg_printf(95)("Error sending fragment\n");
		  repeat = 0;
		  success = 0;
		}
	    }
	  else
	    {
	      repeat = (reply_buf[0] == 1);
	    }
	}
      return success;
    }

  int FReceive()
    {
      _setmx(&mx0,(void*)&tag,GetHeaderSize());
      int pid = Receivemx(0,top+1,&mx0);
      return pid;
    }

  void FReply(int pid, int repeat)
    {
      reply_buf[0] = repeat;
      Replymx(pid,1,&mx_reply);
    }

  // Conservative estimate
  int GetHeaderSize(int ntop=(MAX_FRAGMENT+1))
    {
      ntop = MAX_FRAGMENT+1;
      return ((char*)&lengths[0]-(char*)&tag)+ntop*INT_SIZE;
    }
};
*/


YARPNameID GetServer(const char *name)
{
  return YARPNameService::LocateName(name);
}


int MakeServer(const char *name)
{
  return YARPNameService::RegisterName(name);
}


int Port::SendHelper(YARPNameID pid, const char *buf, int len, int tag)
{
  NewFragmentHeader *p_hdr;
  sender.Begin(pid);
  p_hdr = sender.AddHeader();
  p_hdr->tag = tag;
  p_hdr->length = len;
  p_hdr->more = 0;
  p_hdr->first = 1;
  sender.Add((char*)buf,len);
  return sender.End();
}

int Port::SayServer(YARPNameID pid, const char *buf)
{
  int result = -1;
  if (pid.IsValid())
    {
      result = SendHelper(pid,buf,strlen(buf)+1);
    }
  return result;
}

YARPNameID Port::GetServer(const char *name)
{
  return ::GetServer(name);
}

int Port::MakeServer(const char *name)
{
  return ::MakeServer(name);
}


void OutputTarget::Body()
{
  int success;
  NewFragmentHeader header;
  BlockSender sender;
  CountedPtr<Sendable> p_local_sendable;


  YARPNameID target_pid = GetServer(GetLabel().c_str());
  if (!target_pid.IsValid())
    {
      WaitMutex();
      active = 0;
      PostMutex();
      while(1)
	{
	  something_to_send.Wait();
	  YARPScheduler::Yield();
#ifdef UPDATED_PORT
      space_available.Post();
#endif
	}
    }
  while(1)
    {
      dbg_printf(80)("Waiting for sema to send\n");
      something_to_send.Wait();
      if (!deactivate)
	{
	  dbg_printf(80)("Done waiting for sema to send\n");
	  WaitMutex();
	  check_tick = YARPTime::GetTimeAsSeconds();
	  ticking = 1;
	  p_local_sendable.Set(p_sendable.Ptr());
	  p_sendable.Reset();
	  PostMutex();
	  dbg_printf(80)("Waiting for sema to send <<< sema okay!\n");
	  sender.Begin(target_pid);
	  header.tag = MSG_ID_DATA;
	  header.length = 0;
	  header.first = 1;
	  header.more = 0;
	  if (add_header)
	    {
	      sender.Add((char*)(&header), sizeof(header));
	    }
	  success = p_local_sendable.Ptr()->Write(sender);
	  DBG(45) if (!success) cout << "*** Fire failed" << endl;
	  success = success && sender.End();
	}
      WaitMutex();
      if (deactivate)
	{
	  active = 0;
	  deactivated = 1;
	}
      if (!success)
	{
	  DBG(45) cout << "*** Send failed" << endl;
	  active = 0;
	}
      sending = 0;
      ticking = 0;
      PostMutex();
      p_local_sendable.Reset();
#ifdef UPDATED_PORT
      space_available.Post();
#endif
      OnSend();
    }
}


void Port::Body()
{
  int pid;
  int failed = 0;
  int tag = 0;
  char *buf;
  Fragments cmd;
  OutputTarget *target, *next;
  BlockReceiver receiver;
  NewFragmentHeader hdr;
  int ok;
  int assume_data;
  int call_on_read = 0;

  MakeServer(name.c_str());
  
  if (asleep)
    {
      asleep = 0;
      okay_to_send.Post();
    }

  while(1)
    {
      receiver.Begin();
      receiver.Get();
      out_mutex.Wait();
      assume_data = !expect_header;
      out_mutex.Post();
      //if (asleep)
      //{
      //wakeup.Post();
      //}
      if (!assume_data)
	{
	  ok = receiver.Get((char*)(&hdr),sizeof(hdr));
	  if (ok || hdr.checker=='/')
	    {
	      if (hdr.checker == '~')
		{
		  tag = hdr.tag;
		  cmd.Require(hdr.length);
		  if (tag != MSG_ID_DATA)
		    {
		      ok = receiver.Get(cmd.GetBuffer(),hdr.length);
		      DBG(95) cout << "Got some form of command ("
			<< ((int)tag) << "," << ok << ")" << endl;
		    }
		}
	      else
		{
		  if (hdr.checker != '/')
		    {
		      cerr << "Error - command received in unknown protocol (" 
			   << name << ")"
			   << endl;
		      ok = 0;
		    }
		  else
		    {
		      dbg_printf(0)("%s received unsupported old format request: %s\n", name.c_str());
		    }
		}
	    }
	  if (!ok)
	    {
	      tag = MSG_ID_ERROR;
	    }

	  if (tag == MSG_ID_NULL)
	    {
	      buf = cmd.GetBuffer();
	      tag = buf[0];
	    }
	  else
	    {
	      buf = cmd.GetBuffer();
	    }
	}
      else
	{
	   dbg_printf(95)("Auto assume data\n");
	  tag = MSG_ID_DATA;
	}

      pid = 0;

      if (tag!=MSG_ID_DATA)
	{
	  receiver.End();
	}

      
      int scanned = 0;
      if (!scanned)
	{
	  double now = YARPTime::GetTimeAsSeconds();
	  target = targets.GetRoot();
	  while (target!=NULL)
	    {
	      next = target->GetMeshNext();
	      target->WaitMutex();
	      int active = target->active;
	      int deactivated = target->deactivated;
	      int ticking = target->ticking;
	      double started = target->check_tick;
	      target->PostMutex();
	      int timeout = 0;
	      if (ticking && now-started>5)
		{
		  active = 0;
		  timeout = 1;
		}
	      if (!active)
		{
		  dbg_printf(40)("Removing connection between %s and %s (%s%s)\n",
				 name.c_str(), target->GetLabel().c_str(),
				 deactivated?
				 "as requested":
				 "target stopped responding",
				 timeout?"/timeout":"");
		  delete target;
		}
	      target = next;
	    }
	  scanned = 1;
	}
      

      
      if (pid!=-1)
	{
	  switch(tag)
	    {
	    case MSG_ID_ATTACH:
	      {
		target = targets.GetByLabel(buf);
		if (target==NULL)
		  {
		    dbg_fprintf(40)(stderr,
				    "Starting connection between %s and %s\n",
				   name.c_str(), buf);
		    target = targets.NewLink(buf);
		    assert(target!=NULL);
		    target->target_pid.Invalidate();
		    target->Begin();
		  }
		else
		  {
		    dbg_printf(45)("Ignoring %s, already connected\n", buf);
		  }
	      }
	      break;

	    case MSG_ID_DETACH:
	      {
		dbg_printf(70)("Received detach request for %s\n", buf+1);
		target = targets.GetByLabel(buf+1);
		if (target!=NULL)
		  {
		    dbg_printf(70)("Removing connection between %s and %s\n", 
				   name.c_str(), target->GetLabel().c_str());
		    target->Deactivate();
		  }
	      }
	      break;

	    case MSG_ID_DATA:
	      {
		dbg_printf(70)("Gosh, someone sent me data!  Me being %s in case you're curious\n", name.c_str());

		out_mutex.Wait();
		receiving = 1;
		while(p_receiver_incoming.Ptr() == NULL)
		  {
		    dbg_printf(70)("%%% Waiting for incoming space\n");
		    // HIT - should have way to convey back skip
		    // request to sender
		    // (can't just ignore, don't know how many)
		    // (components message has)
		    asleep = 1;
		    out_mutex.Post();
		    wakeup.Wait();
		    out_mutex.Wait();
		  }
		asleep = 0;
		p_receiver_incoming.Ptr()->AddRef();
		out_mutex.Post();

		p_receiver_incoming.Ptr()->Read(receiver);
		p_receiver_incoming.Ptr()->RemoveRef();

		receiver.End();
		
		out_mutex.Wait();
		receiving = 0;
		dbg_printf(70)("%%% Received. Switching with latest space\n");
		p_receiver_latest.Switch(p_receiver_incoming);
		if (!has_input)
		  {
		    has_input = 1;
		    something_to_read.Post();
		  }
		out_mutex.Post();
		//OnRead();
		call_on_read = 1;
	      }
	      break;

	    case MSG_ID_GO:
	      {
		// Send message to all listeners
		if (!scanned)
		  {
		    target = targets.GetRoot();
		    while (target!=NULL)
		      {
			next = target->GetMeshNext();
			target->WaitMutex();
			int active = target->active;
			int deactivated = target->deactivated;
			target->PostMutex();
			if (!active)
			  {
			    dbg_printf(40)("Removing connection between %s and %s (%s)\n",
					   name.c_str(), target->GetLabel().c_str(),
					   deactivated?
					   "as requested":
					   "target stopped responding");
			    delete target;
			  }
			target = next;
		      }
		  }
		dbg_printf(95)("Sending a message from %s\n", name.c_str());
		target = targets.GetRoot();
		out_mutex.Wait();
		while (target!=NULL)
		  {
		    dbg_printf(80)("Sending a message from %s to target %s\n",
				   name.c_str(), target->GetLabel().c_str());
		    if (p_sendable.Ptr()!=NULL)
		      {
#ifdef UPDATED_PORT
			if (require_complete_send)
			  {
			    target->space_available.Wait();
			    target->space_available.Post();
			  }
			target->Share(p_sendable.Ptr());
			target->add_header = add_header;
#else
			target->Share(p_sendable.Ptr());
			target->add_header = add_header;
#endif
		      }
		    else
		      {
			dbg_printf(45)("Delayed message skipped\n");
		      }
		    target = target->GetMeshNext();
		  }
		p_sendable.Reset();
		out_mutex.Post();
		/*
		target = targets.GetRoot();
		out_mutex.Wait();
		while (target!=NULL)
		  {
		    next = target->GetMeshNext();
		    dbg_printf(80)("Sending a message to target %s\n",
				   target->GetLabel());
		    // Do the send
		    SendFragments(target->pid,out_fragments,MSG_ID_DATA);
		    if (failed)
		      {
			delete target;
		      }
		    target = next;
		  }
		out_mutex.Post();
		*/
		pending = 0;
		okay_to_send.Post();
	      }
	      break;
	    case MSG_ID_DETACH_ALL:
	      {
		dbg_printf(70)("Received detach_all request (%s)\n", 
			       name.c_str());
		target = targets.GetRoot();
		while (target!=NULL)
		  {
		    next = target->GetMeshNext();
		    dbg_printf(70)("Removing connection between %s and %s\n", 
				   name.c_str(), target->GetLabel().c_str());
		    target->Deactivate();
		    target = next;
		  }
	      }
	      break;

	    case MSG_ID_ERROR:
		dbg_printf(50)("Error message received by %s!\n", name.c_str());
	      break;
	    default:
	      {
		dbg_printf(80)("Unknown message received by %s (tag is %d-->'%c')!\n", name.c_str(), tag, tag);
	      }
	      break;
	    }
	}
      
      if (call_on_read)
	{
	  OnRead();
	  call_on_read = 0;
	}
    }
}



void Port::Share(Sendable *nsendable)
{
#ifdef UPDATED_PORT
  //printf("*** debug 1\n");
  okay_to_send.Wait();
  //printf("*** debug 2\n");
  out_mutex.Wait();
  //printf("*** debug 3\n");
  p_sendable.Set(nsendable);
  //printf("*** debug 4\n");
  out_mutex.Post();
  //printf("*** debug 5\n");
  char buf[2] = {MSG_ID_GO, 0};
  //printf("*** debug 6\n");
  Say(buf);
  //printf("*** debug 7\n");
#else
  out_mutex.Wait();
  p_sendable.Set(nsendable);
  out_mutex.Post();
  Fire();
#endif
}


void Port::TakeReceiverAccess(Receivable *nreceiver)
{
  out_mutex.Wait();
  assert(!accessing);
  p_receiver_access.Set(nreceiver);
  out_mutex.Post();
}


void Port::TakeReceiverLatest(Receivable *nreceiver)
{
  out_mutex.Wait();
  p_receiver_latest.Set(nreceiver);
  out_mutex.Post();
}


void Port::TakeReceiverIncoming(Receivable *nreceiver)
{
  out_mutex.Wait();
  assert(!receiving);
  p_receiver_incoming.Set(nreceiver);
  out_mutex.Post();
}

Sendable *Port::Acquire(int wait)
{
  int go = 0;
  Sendable *result = NULL;
  if (wait)
    {
      out_mutex.Wait();
      while (!has_input)
	{
	  out_mutex.Post();
	  something_to_read.Wait();
	  out_mutex.Wait();
	}
      out_mutex.Post();
      go = 1;
    }
  else
    {
      // HIT unfinished
      go = something_to_read.PollingWait();
    }
  if (go)
    {
      out_mutex.Wait();
      assert(!accessing);
      if (p_receiver_latest.Ptr()!=NULL)
	{
	  dbg_printf(70)("%%% READ from latest space\n");
	  p_receiver_latest.Switch(p_receiver_access);
	  has_input = 0;
	}
      result = p_receiver_access.Ptr();
      if (asleep)
	{
	  wakeup.Post();
	}
      accessing = 1;
      out_mutex.Post();
    }
  return result;
}

void Port::Relinquish()
{
  out_mutex.Wait();
  if (accessing)
    {
      if (p_receiver_incoming.Ptr()==NULL)
	{
	  dbg_printf(70)("%%% READ relinquish to incoming space\n");
	  p_receiver_incoming.Switch(p_receiver_access);
	}
      else if (p_receiver_latest.Ptr()==NULL)
	{
	  dbg_printf(70)("%%% READ relinquish to latest space\n");
	  p_receiver_latest.Switch(p_receiver_access);
	}
      else
	{
	  dbg_printf(70)("%%% READ relinquish to access space\n");
	}
      if (asleep)
	{
	  wakeup.Post();
	}
      accessing = 0;
    }
  out_mutex.Post();
}


Port::~Port()
{
  OutputTarget *target, *next;
  target = targets.GetRoot();
  while (target!=NULL)
    {
      next = target->GetMeshNext();
      target->End();
      delete target;
      target = next;
    }
  End();
}


int Port::IsSending()
{
  //printf("sending?? pending=%d\n", pending);
  int sending = pending;
  OutputTarget *target, *next;
  target = targets.GetRoot();
  while (target!=NULL && !sending)
    {
      target->WaitMutex();
      //printf(">>> sending?? %s->sending=%d\n", target->GetLabel().c_str(), target->sending);
      sending |= target->sending;
      target->PostMutex();
      target = target->GetMeshNext();
    }
  return sending;
}


void Port::FinishSend()
{
  int sending = 0;
  OutputTarget *target, *next;
  target = targets.GetRoot();
  while (pending)
    {
      // wait for a sister process (at same priority level) to catch up
      YARPScheduler::Yield();
    }
  while (target!=NULL)
    {
      target->space_available.Wait();
      target->space_available.Post();
      target = target->GetMeshNext();
    }
}
