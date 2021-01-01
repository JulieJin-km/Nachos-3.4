#include "syscall.h"
int main()
{
    int tid=Exec("../test/halt");
    Join(tid);
}
