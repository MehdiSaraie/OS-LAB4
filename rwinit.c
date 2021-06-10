#include "types.h"
#include "stat.h"
#include "user.h"

#define STDOUT 1

int main(int argc, char *argv[]) {
    if (rwinit() < 0)
        printf(STDOUT, "rwinit failed\n");
        
    printf(STDOUT, "hallelujah %d\n", getpid());
    exit();
}