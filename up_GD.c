#include "types.h"
#include "stat.h"
#include "user.h"

#define STDOUT 1


int main(int argc, char *argv[])
{

    int program_id = getpid();
    printf(STDOUT, "program's pid = %d\n",program_id);

    int n1 = fork();
    if(n1 == 0)  // child
    {
        int n2 = fork();
        if (n2 == 0) // child
        {
            int n3 = fork();
            if (n3 == 0) // child
            {
                int n4 = fork();
                if (n4 == 0) // child
                {
                    printf(STDOUT, "successors of pid %d are:\n", program_id);

                    if (getDescendants(program_id) != 1)
                        printf(STDOUT, "can't find successors!\n");
                }
                else if (n4 > 0) { // parent
                    sleep(4);
                    wait();
                }

                else // error
                    printf(STDOUT, "error\n");
            }
            else if (n3 > 0) { // parent
                sleep(3);
                wait();
            }

            else // error
                printf(STDOUT, "error\n");
        }
        else if (n2 > 0) { // parent
            sleep(2);
            wait();
        }

        else // error
            printf(STDOUT, "error\n");

    }
    else if(n1 > 0) { // parent
        sleep(1);
        wait();
    }

    else // error
        printf(STDOUT, "error\n");

    exit();
}
