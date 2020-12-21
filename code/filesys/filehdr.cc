// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"
#include <time.h>
#include <string.h>

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
	return FALSE;		// not enough space
    if(numSectors<=NumDirect-1){
        for(int i=0;i<numSectors;i++)
            dataSectors[i]=freeMap->Find();
    }
    else{
        for(int i=0;i<NumDirect-1;i++)
            dataSectors[i]=freeMap->Find();
        dataSectors[NumDirect-1]=freeMap->Find();
        int indirectDataSectors[NumIndirect];
        for(int i=0;i<numSectors-NumDirect+1;i++)
            indirectDataSectors[i]=freeMap->Find();
        freeMap->Print();
        synchDisk->WriteSector(dataSectors[NumDirect-1],(char*)indirectDataSectors);
    }
/*
    for (int i = 0; i < numSectors; i++)
	dataSectors[i] = freeMap->Find();*/
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    if(numSectors<=NumDirect-1){    
        for (int i = 0; i < numSectors; i++) {
	    //ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	    freeMap->Clear((int) dataSectors[i]);
        }
    }
    else{
        char *indirectDataSectors=new char[SectorSize];
        synchDisk->ReadSector(dataSectors[NumDirect-1],indirectDataSectors);
        for(int i=0;i<numSectors-NumDirect+1;i++){
            freeMap->Clear((int)indirectDataSectors[i*4]);
        }
        for(int i=0;i<NumDirect;i++){
            freeMap->Clear((int)dataSectors[i]);
        }
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    if(offset<SectorSize*(NumDirect-1)){
        return(dataSectors[offset / SectorSize]);
    }
    else{
        int mysector=(offset-SectorSize*(NumDirect-1))/SectorSize;
        char* indirectDataSectors=new char[SectorSize];
        synchDisk->ReadSector(dataSectors[NumDirect-1],indirectDataSectors);
        return int(indirectDataSectors[mysector*4]);
    }
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:", numBytes);
    if(numSectors<=NumDirect-1){
        for (i = 0; i < numSectors; i++)
	        printf("%d ", dataSectors[i]);
    }
    else{
        printf("indirect:%d\n",dataSectors[NumDirect-1]);
        for(i=0;i<NumDirect-1;i++)
            printf("%d ",dataSectors[i]);
        char *indirectDataSectors=new char[SectorSize];
        synchDisk->ReadSector(dataSectors[NumDirect-1],indirectDataSectors);
        j=0;
        for(i=0;i<numSectors-NumDirect+1;i++){
            printf("%d ",int(indirectDataSectors[j]));
            j=j+4;
        }
    }
    printf("\nType:%s\n",type);
    printf("sector:%d\n",mysector);
    printf("created time:%s\n",time_create);
    printf("last_visit time:%s\n",time_last_visited);
    printf("last_modified time:%s\n",time_last_modified);
    printf("File contents:\n");
    if(numSectors<=NumDirect-1){ 
        for (i = k = 0; i < numSectors; i++) {
	        synchDisk->ReadSector(dataSectors[i], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	        if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		        printf("%c", data[j]);
            else
		        printf("\\%x", (unsigned char)data[j]);
	        }
        }
        printf("\n"); 
    }
    else{
        for(i=k=0;i<NumDirect-1;i++){
            printf("Sector:%d\n",dataSectors[i]);
            synchDisk->ReadSector(dataSectors[i],data);
            for(j=0;(j<SectorSize)&&(k<numBytes);j++,k++){
                if('\040'<=data[j]&&data[j]<='\176')
                    printf("%c",data[j]);
                else
                    printf("\\%x",(unsigned char)data[j]);
            }
            printf("\n");
        }
        char *indirectDataSectors=new char[SectorSize];
        synchDisk->ReadSector(dataSectors[NumDirect-1],indirectDataSectors);
        for(i=0;i<numSectors-NumDirect+1;i++){
            printf("Sector:%d\n",int(indirectDataSectors[i*4]));
            synchDisk->ReadSector(int(indirectDataSectors[i*4]),data);
            for(j=0;(j<SectorSize)&&(k<numBytes);j++,k++){
                if('\040'<=data[j]&&data[j]<='\176')
                    printf("%c",data[j]);
                else 
                    printf("%c",(unsigned char)data[j]);
            }
            printf("\n");
        }
    }
    delete [] data;
}

void 
FileHeader::set_time_create()
{
    time_t nows;
    time(&nows);
    strncpy(time_create,asctime(gmtime(&nows)),25);
    time_create[24]='\0';
}

void 
FileHeader::set_time_last_visited()
{
    time_t nows;
    time(&nows);
    strncpy(time_last_visited,asctime(gmtime(&nows)),25);
    time_last_visited[24]='\0';
}

void
FileHeader::set_time_last_modified()
{
    time_t nows;
    time(&nows);
    strncpy(time_last_modified,asctime(gmtime(&nows)),25);
    time_last_modified[24]='\0';
}

bool
FileHeader::Extend(BitMap *freeMap,int bytes)
{
    numBytes=numBytes+bytes;
    int initial_sector=numSectors;
    numSectors=divRoundUp(numBytes,SectorSize);
    if(initial_sector==numSectors)
        return TRUE;
    if(freeMap->NumClear()<numSectors-initial_sector)
        return FALSE;
    printf("extend %d sectors\n",numSectors-initial_sector);
    if(initial_sector<=NumDirect-1){
        if(numSectors<=NumDirect-1){
            for(int i=initial_sector;i<numSectors;i++)
                dataSectors[i]=freeMap->Find();
        }
        else{
            for(int i=initial_sector;i<NumDirect-1;i++)
                dataSectors[i]=freeMap->Find();
            dataSectors[NumDirect-1]=freeMap->Find();
            int indirectDataSectors[NumIndirect];
            for(int i=0;i<numSectors-NumDirect+1;i++)
                indirectDataSectors[i]=freeMap->Find();
            synchDisk->WriteSector(dataSectors[NumDirect-1],(char*)indirectDataSectors);
        }
    }
    else{
        char* indirectDataSectors=new char[SectorSize];
        synchDisk->ReadSector(dataSectors[NumDirect-1],indirectDataSectors);
        for(int i=initial_sector;i<numSectors;i++)
            ((int*)indirectDataSectors)[i-NumDirect+1]=freeMap->Find();
        synchDisk->WriteSector(dataSectors[NumDirect-1],indirectDataSectors);
    }
    return TRUE;

}
