//
// 
#ifndef __YARPRateThreadh__
#define __YARPRateThreadh__

#ifdef __WIN__
#error "YARPRateThread is not yet supported on WIN32"
#endif

#ifdef __QNX__

#include "YARPSemaphore.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/kernel.h>
#include <sys/proxy.h>
#include <sys/wait.h>
#include <sys/sched.h>
#include <time.h>
#include <signal.h>
#include <process.h>

#ifndef DEFAULT_THREAD_STACK_SIZE
#define DEFAULT_THREAD_STACK_SIZE 8000
#endif

// How to use YARPRateThread:
//	1) Derive from it.
//	2) Override DoInit, DoLoop, DoRelease pure virtual methods.
//	3) Use Start, Terminate to start/stop the thread.
//	4) The constructor takes as argument the period for the timer.
//	5) DoInit is called once after the thread is started.
//	6) DoLoop is called periodically (it waits on the timer).
//	7) DoRelease is called upon calling Terminate.
//
class YARPRateThread {
private:
	YARPRateThread& operator=(const YARPRateThread&);
	YARPRateThread(const YARPRateThread&);

protected:
	char *stack;
	int pid;			// this was the thread id.

	bool isRunningF;	// running flag.	
	int period;			// period [ms].
	const char *name;	// thread name.

	pid_t proxy_end;
	pid_t proxy_synchro;

	YARPSemaphore   mutex;		// mutex. 
	
	timer_t timer;		// waitable timer (a sort of).
	pid_t proxy_timer;
	struct itimerspec timer_spec;
	struct sigevent event;

	virtual void DoInit(void *) =0;
	virtual void DoLoop(void *) =0;
	virtual void DoRelease(void *) =0;

public:
	inline void Lock(void)
	{	
		mutex.Wait ();
	}

	inline void Unlock(void)
	{
		mutex.Post ();
	}

public:
	YARPRateThread(const char *c, int r, int prio=29) : name(c), mutex(1)
	{
		assert (r > 0);

		pid = 0;
		isRunningF = false;
		period = r;

		// this is needed for synchro.
		proxy_synchro = qnx_proxy_attach (0, 0, 0, -1);
		assert (proxy_synchro >= 0);

		stack = NULL;

		// try to go high priority by default.
		// this is because I couldn't fix a problem on quit.
		// this sends actually everything at high priority.
		int ret = setprio (0, prio);
		if (ret < 0)
		{
			cout << "set priority failed... abort" << endl;
			exit (-1);
		}
	}

	virtual ~YARPRateThread(void)
	{
		// close the thread
		if (isRunningF) 
		{
			isRunningF = false;
			// force a terminate.
			Terminate(1);
		}

		qnx_proxy_detach (proxy_synchro);

		// should it wait for the thread to exit.
	}

	virtual void Start(bool wait = true)
	{
		Lock ();

		if (isRunningF)
		{
			Unlock ();
			return;
		}

		stack = new char[DEFAULT_THREAD_STACK_SIZE];
		assert (stack != NULL);
		//pid = tfork(stack, DEFAULT_THREAD_STACK_SIZE, &RealThread, (void*)this, 0);
		pid = _beginthread(&RealThread, stack, DEFAULT_THREAD_STACK_SIZE, (void*)this);

		assert (pid >= 0);
		isRunningF = true;
		
		Unlock ();

		// aspetta che sia completata la routine di start.
		if (wait)
			WaitSynchro ();
	}

	void Terminate(bool wait = true)
	{
		Lock ();
		if (!isRunningF)
		{
			printf ("growl...\n");
			kill(pid, SIGTERM);
			waitpid(pid, NULL, 0);
			pid = -1;
			if (stack != NULL)
			{
				delete[] stack;
				stack = NULL;
			}
			Unlock ();
			return;
		}

		Trigger (proxy_end);
		Unlock ();

		if (wait)
		{
			Receive (proxy_synchro, 0, 0);
		}

		Lock ();
		isRunningF = false;
		if (stack != NULL)
		{
			delete[] stack;
			stack = NULL;
		}
		Unlock ();
	}

	void WaitSynchro(void)
	{
		Receive (proxy_synchro, 0, 0);
	}

	void SetPriority(int p)
	{
		int ret = setprio (0, p);
		if (ret < 0)
		{
			cout << "set priority failed (must be root?)" << endl;
		}
		return;
	}

	void SetInterval(int i)
	{
		assert (i > 0);

		Lock ();
		period = i;

		timer_spec.it_value.tv_sec = 0;
		timer_spec.it_value.tv_nsec = i * 1000000;
		timer_spec.it_interval.tv_sec = 0;
		timer_spec.it_interval.tv_nsec = i * 1000000;

		// start the timer.
		timer_settime (timer, 0, &timer_spec, NULL);

		Unlock ();
	}

	bool IsRunning(void)
	{
		Lock ();
		bool ret = isRunningF;
		Unlock ();
		return ret;
	}
	
	const char *GetName(void) const { return name; }

	static void RealThread(void *args)
	{
		YARPRateThread *thisThread = (YARPRateThread *)args;

		thisThread->Lock ();

		// create proxies
		thisThread->proxy_timer = qnx_proxy_attach (0, 0, 0, -1);
		assert (thisThread->proxy_timer >= 0);
		thisThread->proxy_end = qnx_proxy_attach (0, 0, 0, -1);
		assert (thisThread->proxy_end >= 0);

		thisThread->event.sigev_signo = -thisThread->proxy_timer;
		thisThread->timer = timer_create (CLOCK_REALTIME, &thisThread->event);
		assert (thisThread->timer >= 0);

		// prepare remaining structs.
		long int nsec = (thisThread->period * 1000000) % 1000000000;
		long int sec = (thisThread->period * 1000000) / 1000000000;
		thisThread->timer_spec.it_value.tv_sec = sec;
		thisThread->timer_spec.it_value.tv_nsec = nsec;
		thisThread->timer_spec.it_interval.tv_sec = sec;
		thisThread->timer_spec.it_interval.tv_nsec = nsec;

		// start the timer.
		timer_settime (thisThread->timer, 0, &thisThread->timer_spec, NULL);

		thisThread->Unlock ();

		Receive (thisThread->proxy_timer, 0, 0);

		// call the init function.
		thisThread->DoInit (args);

		// send event.
		Trigger (thisThread->proxy_synchro);

		bool local_end = false; 

		while (!local_end)
		{
			// ret = WaitForSingleObject (timer, INFINITE);
			// _ASSERT (ret == WAIT_OBJECT_0);
			pid_t recv = Receive (0, 0, 0);

			if (recv == thisThread->proxy_end)
			{
				local_end = true;
			}
			else
			if (recv == thisThread->proxy_timer)
			{
				thisThread->DoLoop (args);
			}
			else
			{
				printf ("Something wrong inside a thread\n");
				//return -1;
				_endthread ();
			}
		}

		thisThread->DoRelease (args);

		thisThread->Lock ();
		// timer must be already stopped.
		thisThread->timer_spec.it_value.tv_sec = 0;
		thisThread->timer_spec.it_value.tv_nsec = 0;
		timer_settime (thisThread->timer, 0, &thisThread->timer_spec, NULL);
		timer_delete (thisThread->timer);

		// detach proxies
		qnx_proxy_detach (thisThread->proxy_timer);
		qnx_proxy_detach (thisThread->proxy_end);
		thisThread->Unlock ();

		Trigger (thisThread->proxy_synchro);

		//exit(1);
		_endthread ();
		//return 1;
		// The previous version (see class aThread) had both a checked unlock 
		// through exceptions handling and a termination control (against deadlocks).
		// TODO: find an alternative to structured exceptions (which are slow).
	}
};

#ifdef DEFAULT_THREAD_STACK_SIZE
#undef DEFAULT_THREAD_STACK_SIZE
#endif

#endif

#endif