#include "types.h"
#include "stat.h"
#include "user.h"

#define STDOUT 1

int main(int argc, char *argv[])
{

    printf(STDOUT, "process is created at %d ticks\n",processStartTime());

    exit();
}