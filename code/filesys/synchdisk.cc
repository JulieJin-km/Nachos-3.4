// synchdisk.cc 
//	Routines to synchronously access the disk.  The physical disk 
//	is an asynchronous device (disk requests return immediately, and
//	an interrupt happens later on).  This is a layer on top of
//	the disk providing a synchronous interface (requests wait until
//	the request completes).
//
//	Use a semaphore to synchronize the interrupt handlers with the
//	pending requests.  And, because the physical disk can only
//	handle one operation at a time, use a lock to enforce mutual
//	exclusion.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synchdisk.h"
#include "system.h"

//----------------------------------------------------------------------
// DiskRequestDone
// 	Disk interrupt handler.  Need this to be a C routine, because 
//	C++ can't handle pointers to member functions.
//----------------------------------------------------------------------

static void
DiskRequestDone (int arg)
{
    SynchDisk* disk = (SynchDisk *)arg;

    disk->RequestDone();
}

//----------------------------------------------------------------------
// SynchDisk::SynchDisk
// 	Initialize the synchronous interface to the physical disk, in turn
//	initializing the physical disk.
//
//	"name" -- UNIX file name to be used as storage for the disk data
//	   (usually, "DISK")
//----------------------------------------------------------------------

SynchDisk::SynchDisk(char* name)
{
    semaphore = new Semaphore("synch disk", 0);
    lock = new Lock("synch disk lock");
    disk = new Disk(name, DiskRequestDone, (int) this);
    for(int i=0;i<NumSectors;i++){
        rwmutex[i]=new Semaphore("file",1);
        numReaders[i]=0;
        cntVisitors[i]=0;
    }
    readerLock=new Lock("readerLock");
    
    for(int i=0;i<CacheNum;i++)
        cache[i]=new CacheEntry;
}

//----------------------------------------------------------------------
// SynchDisk::~SynchDisk
// 	De-allocate data structures needed for the synchronous disk
//	abstraction.
//----------------------------------------------------------------------

SynchDisk::~SynchDisk()
{
    delete disk;
    delete lock;
    delete semaphore;
    delete readerLock;
    for(int i=0;i<NumSectors;i++)
        delete rwmutex[i];
    
    for(int i=0;i<CacheNum;i++)
        delete cache[i];
}

//----------------------------------------------------------------------
// SynchDisk::ReadSector
// 	Read the contents of a disk sector into a buffer.  Return only
//	after the data has been read.
//
//	"sectorNumber" -- the disk sector to read
//	"data" -- the buffer to hold the contents of the disk sector
//----------------------------------------------------------------------

void
SynchDisk::ReadSector(int sectorNumber, char* data)
{
    lock->Acquire();// only one disk I/O at a time
    
    int pos=-1;
    for(int i=0;i<CacheNum;i++){
        if(cache[i]->valid==1&&cache[i]->sector==sectorNumber){
            pos=i;
            break;
        }
    }
    if(pos==-1){
        printf("cache miss\n");
        disk->ReadRequest(sectorNumber,data);
        semaphore->P();
        int swap=-1;
        for(int i=0;i<CacheNum;i++){
            if(cache[i]->valid==0){
                swap=i;
                break;
            }
        }
        if(swap==-1){
            int min_time=cache[0]->lru_time;
            int min_pos=0;
            for(int i=1;i<CacheNum;i++){
                if(cache[i]->lru_time<min_time){
                    min_time=cache[i]->lru_time;
                    min_pos=i;
                }
            }
            swap=min_pos;
        }
        cache[swap]->valid=1;
        cache[swap]->dirty=0;
        cache[swap]->sector=sectorNumber;
        cache[swap]->lru_time=stats->totalTicks;
        bcopy(data,cache[swap]->data,SectorSize);
    }
    else{
        printf("cache hit\n");
        cache[pos]->lru_time=stats->totalTicks;
        bcopy(cache[pos]->data,data,SectorSize);
    }
    //disk->ReadRequest(sectorNumber, data);
    //semaphore->P();			// wait for interrupt
    lock->Release();
}

//----------------------------------------------------------------------
// SynchDisk::WriteSector
// 	Write the contents of a buffer into a disk sector.  Return only
//	after the data has been written.
//
//	"sectorNumber" -- the disk sector to be written
//	"data" -- the new contents of the disk sector
//----------------------------------------------------------------------

void
SynchDisk::WriteSector(int sectorNumber, char* data)
{
    lock->Acquire();			// only one disk I/O at a time
    disk->WriteRequest(sectorNumber, data);
    semaphore->P();			// wait for interrupt
    
    for(int i=0;i<CacheNum;i++){
        if(cache[i]->sector==sectorNumber)
            cache[i]->valid=0;
    }
    lock->Release();
}

//----------------------------------------------------------------------
// SynchDisk::RequestDone
// 	Disk interrupt handler.  Wake up any thread waiting for the disk
//	request to finish.
//----------------------------------------------------------------------

void
SynchDisk::RequestDone()
{ 
    semaphore->V();
}

void
SynchDisk::AddReader(int sector)
{
    readerLock->Acquire();
    numReaders[sector]++;
    if(numReaders[sector]==1)
        rwmutex[sector]->P();
    printf("reader count: %d\n",numReaders[sector]);
    readerLock->Release();
}

void
SynchDisk::SubReader(int sector)
{
    readerLock->Acquire();
    numReaders[sector]--;
    if(numReaders[sector]==0)
        rwmutex[sector]->V();
    printf("reader count: %d\n",numReaders[sector]);
    readerLock->Release();
}

void
SynchDisk::BeginWrite(int sector)
{
    rwmutex[sector]->P();
}
void 
SynchDisk::EndWrite(int sector)
{
    rwmutex[sector]->V();
}
