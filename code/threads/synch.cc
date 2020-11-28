// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    DEBUG('t',"Entering P,disable interrupt\n");
    //printf("entering P,value=%d\n",value);
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
    DEBUG('t',"Leaving P,enable interrupt\n");
    //printf("leaving P,value=%d\n",value);
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
   // printf("next thread:%s\n",thread->getName());
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!

//---------------------------------------------------------------------
//Lock::Lock
// Initialize lock to be FREE
// debugName is useful for debugging
// using semaphore to implement it
//---------------------------------------------------------------------
Lock::Lock(char* debugName) 
{
    name=debugName;
    mutex=new Semaphore(debugName,1);
    cur=NULL;
}
//---------------------------------------------------------------------
//Lock::~Lock
// Deallocate the lock.Assume that no one is still waiting for it.
//---------------------------------------------------------------------
Lock::~Lock()
{
    name=NULL;
    delete mutex;
    cur=NULL;
}
//--------------------------------------------------------------------
//Lock::Acquire
// get the lock,if it is taken, then wait for it
//--------------------------------------------------------------------
void 
Lock::Acquire()
{
    mutex->P();
    cur=currentThread;
}
//--------------------------------------------------------------------
//Lock::Release
// release the lock,wake up one is there are any
//--------------------------------------------------------------------
void 
Lock::Release()
{
    mutex->V();
    cur=NULL;
}
//--------------------------------------------------------------------
//Lock::isHeldByCurrentThread
// check if the lock is hold by current thread
//--------------------------------------------------------------------
bool
Lock::isHeldByCurrentThread()
{
    if(cur==currentThread)return true;
    else return false;
}
//-------------------------------------------------------------------
//Condition::Condition
// Initialize condition to no one waiting

//-------------------------------------------------------------------
Condition::Condition(char* debugName)
{
    name=debugName;
    waitqueue=new List();
}
//-------------------------------------------------------------------
//Condition::~Condition
// deallocate the condition
//-------------------------------------------------------------------
Condition::~Condition()
{
    delete waitqueue;
}
//-------------------------------------------------------------------
//Condition::Wait(Lock* conditionLock)
// releasing the lock and going to sleep,when signaled regain the lock
//-------------------------------------------------------------------
void 
Condition::Wait(Lock* conditionLock)
{
    ASSERT(conditionLock->isHeldByCurrentThread());
    conditionLock->Release();
    waitqueue->Append(currentThread);
    currentThread->Sleep();
    conditionLock->Acquire();
}
//-------------------------------------------------------------------
//Condition::Signal(Lock* conditionLock)
// wake up a corresponding thread
//-------------------------------------------------------------------
void
Condition::Signal(Lock* conditionLock)
{
    ASSERT(conditionLock->isHeldByCurrentThread());
    if(!waitqueue->IsEmpty())
    {
        Thread* nextThread=(Thread*)waitqueue->Remove();
        scheduler->ReadyToRun(nextThread);
    }
}
//------------------------------------------------------------------
//Condition::Broadcast(Lock* conditionLock)
// wake up all corresponding threads
//------------------------------------------------------------------
void 
Condition::Broadcast(Lock* conditionLock)
{
    ASSERT(conditionLock->isHeldByCurrentThread());
    while(!waitqueue->IsEmpty())
    {
        Thread* nextThread=(Thread*)waitqueue->Remove();
        scheduler->ReadyToRun(nextThread);
    }
}

RWLock::RWLock(char*debugName)
{
    name=debugName;
    readers=0;
    rw=new Lock("sem");
    mutex=new Lock("mutex");
}

RWLock::~RWLock()
{
    name=NULL;
    delete rw;
    delete mutex;
}

void
RWLock::RAcquire()
{
    mutex->Acquire();
    readers++;
    if(readers==1)rw->Acquire();
    mutex->Release();
}

void
RWLock::RRelease()
{
    mutex->Acquire();
    readers--;
    if(readers==0)rw->Release();
    mutex->Release();
}

void
RWLock::WAcquire()
{
    rw->Acquire();
}

void
RWLock::WRelease()
{
    rw->Release();
}
    
