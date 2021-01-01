#include "syscall.h"
void func()
{
    Create("test1.txt");
    Exit(0);
}
int main()
{
    Create("test2.txt");
    Fork(func);
}
