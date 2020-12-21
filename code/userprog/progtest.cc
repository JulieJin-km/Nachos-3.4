// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"

//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------
void ForkThread(int which)
{
    printf("start %d thread\n",which);
    machine->Run();
}
void
StartProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;
    //OpenFile *executable2=fileSystem->Open(filename);
    //AddrSpace *space2;
    //Thread*thread=Thread::cap_Thread("second thread");
    if (executable == NULL) {
	printf("Unable to open file %s\n", filename);
	return;
    }
    printf("initial first thread address space\n");
    space = new AddrSpace(executable);    
    //printf("initial second thread address space\n");
    //space2=new AddrSpace(executable);
    currentThread->space = space;

    //delete executable2;
    //space2->InitRegisters();
    //space2->RestoreState();
    //thread->space=space2;
    //thread->Fork(ForkThread,2);
    //currentThread->Yield();
    delete executable;			// close file

    space->InitRegisters();		// set the initial register values
    space->RestoreState();		// load page table register
    printf("start first thread\n");
    machine->Run();			// jump to the user progam
    ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}


// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail=new Semaphore("read avail",0);
static Semaphore *writeDone=new Semaphore("write done",0);

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------
class SynchConsole{
    public:
        SynchConsole(char* readFile,char* writeFile){
            lock=new Lock("console");
            console=new Console(readFile,writeFile,ReadAvail,WriteDone,0);
        }
        ~SynchConsole(){
            delete lock;
            delete console;
        }
        void PutChar(char ch){
            lock->Acquire();
            console->PutChar(ch);
            writeDone->P();
            lock->Release();
        }
        char GetChar(){
            lock->Acquire();
            readAvail->P();
            char ch=console->GetChar();
            lock->Release();
            return ch;
        }
    private:
        Console* console;
        Lock* lock;
};

void 
ConsoleTest (char *in, char *out)
{
    char ch;

    SynchConsole *synchconsole = new SynchConsole(in, out);
    //readAvail = new Semaphore("read avail", 0);
    //writeDone = new Semaphore("write done", 0);
    
    for (;;) {
	//readAvail->P();		// wait for character to arrive
	ch = synchconsole->GetChar();
	synchconsole->PutChar(ch);	// echo it!
	//writeDone->P() ;        // wait for write to finish
	if (ch == 'q') return;  // if q, quit
    }
}
