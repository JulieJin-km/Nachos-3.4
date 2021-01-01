// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
void TLBMissHandler();
int PageMissHandler();
void exec_func(int addr);
void fork_func(int addr);
void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
    printf("hit-rate %.3f,miss-rate %.3f\n",(double)machine->hit/machine->total,(double)machine->miss/machine->total);
   	interrupt->Halt();
    } 
    else if(which==PageFaultException){
        if(machine->tlb!=NULL){
            TLBMissHandler();
        }
        else{
            PageMissHandler();
        }
    }
    else if((which==SyscallException)&&(type==SC_Create)){
        printf("Syscall Create\n");
        int addr=machine->ReadRegister(4);
        char name[20];
        int pos=0;
        int buffer;
        while(pos<20){
            machine->ReadMem(addr+pos,1,&buffer);
            if(buffer!=0)
                name[pos++]=(char)buffer;
            else{
                name[pos]='\0';
                break;
            }
        }
        printf("name is %s\n",name);
        fileSystem->Create(name,128);
        machine->PC_INC();
    }
    else if((which==SyscallException)&&(type==SC_Open)){
        printf("Syscall Open\n");
        int addr=machine->ReadRegister(4);
        char name[20];
        int pos=0;
        int buffer;
        while(pos<20){
            machine->ReadMem(addr+pos,1,&buffer);
            if(buffer!=0)
                name[pos++]=(char)buffer;
            else{
                name[pos]='\0';
                break;
            }
        }
        printf("name is %s\n",name);
        OpenFile *openfile=fileSystem->Open(name);
        machine->WriteRegister(2,(int)openfile);
        printf("fd is %d\n",(int)openfile);
        machine->PC_INC();
    }
    else if((which==SyscallException)&&(type==SC_Close)){
        printf("Syscall Close\n");
        int fd=machine->ReadRegister(4);
        OpenFile *openfile=(OpenFile*)fd;
        delete openfile;
        machine->PC_INC();
    }
    else if((which==SyscallException)&&(type==SC_Write)){
        printf("Syscall Write\n");
        int bufferaddr=machine->ReadRegister(4);
        int size=machine->ReadRegister(5);
        int fd=machine->ReadRegister(6);
        char buffer[size];
        int data;
        for(int i=0;i<size;i++){
            machine->ReadMem(bufferaddr+i,1,&data);
            buffer[i]=(char)data;
        }
        OpenFile *openfile=(OpenFile*)fd;
        openfile->Write(buffer,size);
        machine->PC_INC();
    }
    else if((which==SyscallException)&&(type==SC_Read)){
        printf("Syscall Read\n");
        int bufferaddr=machine->ReadRegister(4);
        int size=machine->ReadRegister(5);
        int fd=machine->ReadRegister(6);
        char buffer[size];
        OpenFile *openfile=(OpenFile*)fd;
        printf("size=%d,fd=%d\n",size,fd);
        int curread=openfile->Read(buffer,size);
        printf("current read is %d\n",curread);
        for(int i=0;i<curread;i++){
            machine->WriteMem(bufferaddr+i,1,int(buffer[i]));
        }
        machine->WriteRegister(2,curread);
        machine->PC_INC();
    }
    else if((which==SyscallException)&&(type==SC_Exec)){
        printf("Syscall Exec\n");
        int addr=machine->ReadRegister(4);
        Thread* newthread=Thread::cap_Thread("a new thread");
        newthread->Fork(exec_func,addr);
        machine->WriteRegister(2,newthread->getTid());
        machine->PC_INC();
    }
    else if((which==SyscallException)&&(type==SC_Fork)){
        printf("Syscall Fork\n");
        int addr=machine->ReadRegister(4);
        AddrSpace *space=new AddrSpace(currentThread->space->exeName);
        //for(int i=0;i<NumPhysPages;i++)
            //space->pageTable[i]=currentThread->space->pageTable[i];
        Thread *t=Thread::cap_Thread("fork thread");
        t->space=space;
        t->Fork(fork_func,addr);
        DEBUG('a',"before PC_INC\n");
        machine->PC_INC();
    }
    else if((which==SyscallException)&&(type==SC_Yield)){
        printf("Syscall Yield\n");
        machine->PC_INC();
        currentThread->Yield();
    }
    else if((which==SyscallException)&&(type==SC_Join)){
        printf("Syscall Join\n");
        int tid=machine->ReadRegister(4);
        while(tid_used[tid])
            currentThread->Yield();
        machine->WriteRegister(2,tid_used[tid]);
        machine->PC_INC();
    }
    else if((which==SyscallException)&&(type==SC_Exit)){
        printf("program exit\n");
        /*
        for(int i=0;i<machine->pageTableSize;i++){
            printf("physical page %d:",i);
            if(machine->pageTable[i].valid){
                printf("page used, virtual page %d\n",machine->pageTable[i].virtualPage);
            }
            else printf("page unused\n");
        }*/
        int status=machine->ReadRegister(4);
        printf("program exit with status %d\n",status);
        machine->clear();
        machine->PC_INC();
        currentThread->Finish();
    }
    else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}

void
TLBMissHandler()
{
    int addr=machine->ReadRegister(BadVAddrReg);
    unsigned int vpn,offset;
    int i=-1;
    vpn=(unsigned)addr / PageSize;
    offset=(unsigned)addr % PageSize;
    //finding in pagetable  
    for(int j=0;j<TLBSize;j++){
        if(!machine->tlb[j].valid)
        {
            i=j;
            //printf("in find %d\n",i);
            break;
        }
    }
    if(i==-1)
    {
        for(int j=0;j<TLBSize;j++)
            if(machine->counts[j]==TLBSize){
                i=j;
                //printf("in replace %d",i);
                break;
            }
    }
    //for(int j=0;j<TLBSize;j++)
        //printf("%d ",machine->counts[j]);
    //printf("\n");
    machine->counts[i]=1;
    for(int j=0;j<TLBSize;j++){
        if(j==i)continue;
        if(machine->counts[j]==0)continue;
        machine->counts[j]++;
    }
    //for(int j=0;j<TLBSize;j++)
        //printf("%d ",machine->counts[j]);
    //printf("\n");
    /*
    if(i==-1)
    {
        i=TLBSize-1;
        for(int j=0;j<TLBSize-1;j++)
            machine->tlb[j]=machine->tlb[j+1];
    }*/
    //if(!machine->pageTable[vpn].valid)
    //{
       // PageMissHandler();
    //}
    int corr=-1;
    for(int j=0;j<machine->pageTableSize;j++){
        if(machine->pageTable[j].virtualPage==vpn)
        {
            if(machine->pageTable[j].valid==false)
                corr=PageMissHandler();
            else corr=j;
            break;
        }
    }
    if(corr==-1)corr=PageMissHandler();
    DEBUG('a',"corr=%d\n",corr);
    machine->tlb[i].valid=true;
    machine->tlb[i].virtualPage=vpn;
    machine->tlb[i].physicalPage=machine->pageTable[corr].physicalPage;
    machine->tlb[i].use=false;
    machine->tlb[i].dirty=false;
    machine->tlb[i].readOnly=false;
}

int 
PageMissHandler()
{
    OpenFile *openfile=fileSystem->Open("swapping_area");
    //printf("in handler:size is %d\n",openfile->Length());
    if(openfile==NULL)ASSERT(false);
    int addr=machine->ReadRegister(BadVAddrReg);
    unsigned int vpn;
    vpn=(unsigned)addr/PageSize;
    int empty=machine->find();
    if(empty==-1){
        empty=0;
        for(int j=0;j<machine->pageTableSize;j++){
            if(machine->pageTable[j].physicalPage==0){
                if(machine->pageTable[j].dirty==true){
                    openfile->WriteAt(&(machine->mainMemory[empty*PageSize]),PageSize,machine->pageTable[j].virtualPage*PageSize);
                    machine->pageTable[j].valid=false;
                    break;
                }
            }
        }
    }
    //printf("page fault vpn %d chooses %d\n",vpn,empty);
    openfile->ReadAt(&(machine->mainMemory[empty*PageSize]),PageSize,vpn*PageSize);
    machine->pageTable[empty].valid=true;
    machine->pageTable[empty].virtualPage=vpn;
    machine->pageTable[empty].use=false;
    machine->pageTable[empty].dirty=false;
    machine->pageTable[empty].readOnly=false;
    machine->pageTable[empty].physicalPage=empty;
    //printf("mainMemory[%d]:%x\n",empty*PageSize,machine->mainMemory[empty*PageSize]);
    delete openfile;
    return empty;
}

void
exec_func(int addr)
{
    char name[20];
    int pos=0;
    int buffer;
    while(pos<20){
        machine->ReadMem(addr+pos,1,&buffer);
        if(buffer==0){
            name[pos]='\0';
            break;
        }
        name[pos++]=(char)buffer;
    }
    //printf("in exec_func:%s\n",name);
    AddrSpace *space=new AddrSpace(name);
    //printf("end of space\n");
    space->InitRegisters();
    space->RestoreState();
    currentThread->space=space;
    //printf("before run\n");
    machine->Run();
}

void
fork_func(int addr)
{
    DEBUG('a',"addr is %d\n",addr);
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();
    machine->WriteRegister(PCReg,addr);
    machine->WriteRegister(NextPCReg,addr+4);
    machine->Run();
}
