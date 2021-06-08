#include "types.h"
#include "stat.h"
#include "user.h"

#define STDOUT 1

int main(int argc, char *argv[])
{
    if(argc != 2) {
        printf(STDOUT, "bad argument!\n");
        exit();
    }
    
    
    if (setSleep(atoi(argv[1])) < 0)
        printf(STDOUT, "killed\n");

    exit();
}