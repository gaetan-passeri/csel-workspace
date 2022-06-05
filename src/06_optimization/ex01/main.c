#include <stdint.h>
#include <stdio.h>
#include <time.h>

struct timespec t1;
struct timespec t2;

#define SIZE 5000



static int32_t array[SIZE][SIZE];

int main (void)
{
    clock_gettime(CLOCK_MONOTONIC, &t1);
    int i, j, k;

    for (k = 0; k < 10; k++)
    {
        for (i = 0; i < SIZE; i++)
        {
            for (j = 0; j < SIZE; j++)
            {
                    // array[j][i] += 10;
                array[j][i] ++;
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &t2);

    long long int delta_t = (long long)(t2.tv_sec - t1.tv_sec) * 1000000000
                          + t2.tv_nsec - t1.tv_nsec;
    printf ("elapsed time: %lld [ns]\n", delta_t);
    return 0;
}
