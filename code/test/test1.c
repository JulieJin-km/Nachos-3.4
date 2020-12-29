#include "syscall.h"

int f1,f2;
int curread;
char buffer[100];
int main(){
    Create("file2.txt");
    f1=Open("file1.txt");
    f2=Open("file2.txt");
    curread=Read(buffer,100,f1);
    Write(buffer,curread,f2);
    Close(f1);
    Close(f2);
    Halt();
}
