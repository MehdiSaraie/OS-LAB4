#include "types.h"
#include "stat.h"
#include "user.h"

#define STDOUT 1


int main(int argc, char *argv[])
{

    printf(STDOUT, "program's pid = %d\n",getpid());

    int n1 = fork();
    if(n1 == 0)  // child
    {
        int n2 = fork();
        if (n2 == 0) // child
        {
            printf(STDOUT, "ancestors of pid %d are:\n", getpid());

            if (getAncestors(getpid()) != 1)
                printf(STDOUT, "can't find ancestors!\n");
        }
        else if (n2 > 0) // parent
            wait();

        else // error
            printf(STDOUT, "error\n");

    }
    else if(n1 > 0) // parent
        wait();

    else // error
        printf(STDOUT, "error\n");

    exit();
}