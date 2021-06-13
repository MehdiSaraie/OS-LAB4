#include "types.h"
#include "stat.h"
#include "user.h"

#define STDOUT 1

int* decToBinary(int n) {

    int *out = (int *)malloc(32 * sizeof(int));

    for (int i = 31; i >= 0; i--) {
        int k = n >> i;
        *(out+i) = k & 1;
    }
    return out;
}

int calcLength(int dec) {
    int holder = 1;
    for (int i = 0; i < 32; i++) {
        if (dec < holder) {
            return i;
        }
        holder *= 2;
    }

    return -1;
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf(STDOUT, "bad argument\n");
        exit();
    }

    if (rwinit() < 0)
        printf(STDOUT, "rwinit failed\n");
/////////////////////////////////////////////////////////////////////////
    int *dec = decToBinary(atoi(argv[1]));
    int length = calcLength(atoi(argv[1]));
/////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < length - 1; i++) {
        int n = fork();
        if (n == 0) {   // child
            if (rwtest(dec[length - i - 2], atoi(argv[2])) < 0) {
                printf(STDOUT, "rwtest failed\n");
                exit();
            }
            break;
        }
        else if (n > 0) {   // parent
            sleep(10);
        }
        else {  // error
            printf(STDOUT, "fork failed\n");
            exit();
        }
    }

    while (wait() != -1)
        ;

    exit();
}
