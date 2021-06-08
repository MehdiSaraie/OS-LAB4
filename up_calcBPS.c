#include "types.h"
#include "stat.h"
#include "user.h"

#define STDOUT 1

int main(int argc, char *argv[])
{

    int holder;
    asm("\t movl %%ebx,%0" : "=r"(holder));

    asm ("movl $230, %ebx \n");
    printf(STDOUT, "answer = %d\n",calculateBPS());

    asm("\t movl %0,%%ebx" :: "r"(holder));  // restore

    exit();
}