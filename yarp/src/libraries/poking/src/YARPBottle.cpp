#include <vector>
#include <assert.h>
#include <stdio.h>
#include <iostream.h>

#include "YARPBottle.h"
#include "YARPPort.h"
#include "YARPSemaphore.h"
#include "YARPTime.h"

typedef YARPOutputPortOf<YARPBottle> OutBot;

class SendBottleState
{
public:
  OutBot *out_bot;
  YARPSemaphore mutex;
  
  SendBottleState() : mutex(1) { out_bot = NULL; }

  ~SendBottleState() { if (out_bot!=NULL) delete out_bot; }

  char buf[256];

  void Create() 
    { 
      if (out_bot==NULL) 
	{
	  out_bot = new OutBot;
	  assert(out_bot!=NULL);
	  double t = YARPTime::GetTimeAsSeconds();
	  sprintf(buf,"/log/out/%012.6f/o:bot", t);
	  //sprintf(buf,"/hello");
	  out_bot->Register(buf);
	  //YARPTime::DelayInSeconds(1);
	  out_bot->Connect("/log/i:bot");
	  //YARPTime::DelayInSeconds(1);
	}
    }

  void Write(const YARPBottle& bottle)
    {
      mutex.Wait();
      Create();
      out_bot->Content() = bottle;
      YARPBottleIterator it(out_bot->Content());
      //it.Rewind();
      //it.WriteVocab(YBC_GAZE_FIXATE);
      //it.WriteInt(32);
      out_bot->Write(1);
//      YARPTime::DelayInSeconds(1);
      mutex.Post();
    }
};

static SendBottleState bot_state;

void SendBottle(YARPBottle& bottle)
{
  bot_state.Write(bottle);
}


