// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "synch.h"
// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    int circletime=10;
    
    for (num = 0; num < circletime; num++) {
	printf("*** thread %d with priority %d looped %d times\n", which,currentThread->getPrio(), num+1);
        //currentThread->Yield();
    }
}
void
SimpleThread2(int which)
{
    int num;
    int circletime=10;
    for(num=0;num<circletime;num++){
        printf("*** thread %d with priority %d looped %d times\n"
                ,which,currentThread->getPrio(),num+1);
        if(num==4)
        {
            Thread *t2=Thread::cap_Thread("thread2",1);
            t2->Fork(SimpleThread,t2->getTid());
        }
    }
}
void
SimpleThread3(int time)
{
    int num;
    int circletime=time;
    for(num=0;num<time;num+=10)
    {
        printf("*** %s with usedtime %d and timeslice %d\n",
                currentThread->getName(),num,currentThread->getUsedtime());
        currentThread->advanceTime();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
//  Set up a ping-pong between two threads, by forking a thread 
//  to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t',"Entering ThreadTest1");
    int maxnum=130;

    for(int i=0;i<maxnum;i++)
    {
        Thread *t=Thread::cap_Thread("forked thread");
        if(t==NULL)printf("NULL\n");
        else t->Fork(SimpleThread,t->getTid());
    }
    printf("parent:");
    SimpleThread(currentThread->getTid());
}
//----------------------------------------------------------------------
// TS()
//  print information about all threads in system.
//----------------------------------------------------------------------
void
TS()
{
    for(int i=0;i<128;i++)
    {
        if(threads[i]!=NULL)
        {
            printf("***thread ID %d,   name %s,",threads[i]->getTid()
                    ,threads[i]->getName()
                    );
            switch(threads[i]->getStatus())
            {
                case JUST_CREATED:printf("   status just_created\n");
                                  break;
                case RUNNING:printf("   status running\n");
                             break;
                case READY:printf("   status ready\n");
                           break;
                default:printf("   status blocked\n");
            }
        }
    }
}
//----------------------------------------------------------------------
// PrioTest
// fork 3 threads with different priority to see if preemption happenes
//----------------------------------------------------------------------
void
PrioTest()
{
    DEBUG('t',"Entering the PrioTest");
    Thread *t1=Thread::cap_Thread("thread1",3);
    Thread *t2=Thread::cap_Thread("thread2",2);
    Thread *t3=Thread::cap_Thread("thread3",1);
    t1->Fork(SimpleThread,t1->getTid());
    t2->Fork(SimpleThread,t2->getTid());
    t3->Fork(SimpleThread,t3->getTid());
}
//----------------------------------------------------------------------
// PreemTest
// create thread1 to loop ten times, create thread2 with higher priority
// when thread1 has looped 5 times, to see if preemption happens.
//----------------------------------------------------------------------
void
PreemTest()
{
    DEBUG('t',"Entering the PreemTest");
    Thread *t1=Thread::cap_Thread("thread1",3);
    t1->Fork(SimpleThread2,t1->getTid());
}
//----------------------------------------------------------------------
// RRTest
// create 3 threads to test RR simulation
//----------------------------------------------------------------------
void
RRTest()
{
    Thread *t1=Thread::cap_Thread("thread1");
    Thread *t2=Thread::cap_Thread("thread2");
    Thread *t3=Thread::cap_Thread("thread3");
    t1->Fork(SimpleThread3,80);
    t2->Fork(SimpleThread3,40);
    t3->Fork(SimpleThread3,60);
    //currentThread->advanceTime();
}
//----------------------------------------------------------------------
// IPC problem:producer-consumer
// implemention using semaphores
//----------------------------------------------------------------------
/*
#define N 2    //the size of limited buffer
Semaphore* mutex=new Semaphore("mutex",1);
Semaphore* empty=new Semaphore("empty",N);
Semaphore* full=new Semaphore("full",0);
Semaphore* im=new Semaphore("mutex_item",1);
List* items;
int item=0;
void producer(int number)
{
    im->P();
    while(item<5)
    {
        item++;
        int*tem=new int(item);
        printf("item=%d,by %s\n",item,currentThread->getName());
        im->V();
        DEBUG('t',"***ready to P(empty)\n");
        empty->P();
        DEBUG('t',"***P(emtpy) succeed\n");       
        mutex->P();
        DEBUG('t',"***P(mutex) succeed\n");       
        items->Append((void*)tem);
        printf("producer %d produce item %d\n",number,*tem);
        mutex->V();       
        full->V();
        im->P();
    }
    im->V();
}
void consumer(int number)
{
    int* itemget;
    while(true)
    {
        full->P();
        DEBUG('t',"***P(full) succeed\n");
        mutex->P();
        DEBUG('t',"***P(mutex) succeed\n");
        itemget=(int*)items->Remove();
        printf("consumer %d consume item %d\n",number,*itemget);
        mutex->V();
        empty->V();
    }
}
*/
void producer(int number);
void consumer(int number);
List* items;
void PCtest()
{
    items=new List();
    Thread *p1=Thread::cap_Thread("producer1");
    Thread *p2=Thread::cap_Thread("producer2");
    Thread *p3=Thread::cap_Thread("producer3");
    Thread *p4=Thread::cap_Thread("producer4");
    Thread *c1=Thread::cap_Thread("consumer1");
    Thread *c2=Thread::cap_Thread("consumer2");
    Thread *c3=Thread::cap_Thread("consumer3");
    p1->Fork(producer,1);
    p2->Fork(producer,2);
    p3->Fork(producer,3);
    p4->Fork(producer,4);
    c1->Fork(consumer,1);
    c2->Fork(consumer,2);
    c3->Fork(consumer,3);
}
//----------------------------------------------------------------------
//IPC problems:producer-consumer
// implementation using condition variable
//----------------------------------------------------------------------
#define N 2
Lock* mutex=new Lock("mutex");
Condition *full=new Condition("full");
Condition *empty=new Condition("empty");
Lock* im=new Lock("im");
int item=0;
int buffer=0;
void producer(int number)
{
    im->Acquire();
    while(item<8)
    {
        item++;
        int *tem=new int(item);
        im->Release();
        printf("item=%d,by %s\n",*tem,currentThread->getName());
        mutex->Acquire();
        DEBUG('t',"get mutex\n");
        while(buffer==N)empty->Wait(mutex);
        DEBUG('t',"get buffer\n");
        items->Append((void*)tem);
        DEBUG('t',"insert succeed\n");
        buffer++;
        printf("producer %d produce item %d\n",number,*tem);
        full->Signal(mutex);
        mutex->Release();
        im->Release();
    }
}
void consumer(int number)
{
    int *itemget;
    while(true)
    {
        mutex->Acquire();
        while(buffer==0)full->Wait(mutex);
        itemget=(int*)items->Remove();
        buffer--;
        printf("consumer %d consume item %d\n",number,*itemget);
        empty->Signal(mutex);
        mutex->Release();
    }
}
//----------------------------------------------------------------------
//Read-write lock test
//----------------------------------------------------------------------
RWLock* rwlock=new RWLock("test");
int data=0;
void reader(int number)
{
    rwlock->RAcquire();
    printf("reader %d read %d\n",number,data);
    rwlock->RRelease();
}
void writer(int number)
{
    rwlock->WAcquire();
    printf("writer %d write %d\n",number,number);
    data=number;
    rwlock->WRelease();
}
void RWtest()
{
    Thread*readers[3];
    Thread*writers[3];
    for(int i=0;i<3;i++)
    {
        readers[i]=Thread::cap_Thread("readers");
    }
    for(int i=0;i<3;i++)
    {
        writers[i]=Thread::cap_Thread("writers");
    }
    readers[0]->Fork(reader,0);
    writers[0]->Fork(writer,1);
    readers[1]->Fork(reader,1);
    readers[2]->Fork(reader,2);
    writers[1]->Fork(writer,1);
    writers[2]->Fork(writer,2);
    
}
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum){
    case 7:
        RWtest();
        break;
    case 6:
        PCtest();
        break;
    case 5:
        RRTest();
        break;
    case 4:
        PreemTest();
        break;
    case 3:
        PrioTest();
        break;
    case 2:
    TS();
    ThreadTest1();
    TS();
    break;
    case 1:
	ThreadTest1();
	break;
    default:
	printf("No test specified.\n");
	break;
    }
}

