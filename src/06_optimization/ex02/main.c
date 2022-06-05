#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct timespec t1;
struct timespec t2;

#define SIZE 65536

static int compare (const void* a, const void* b)
{
    return *(short*)a - *(short*)b;
}

long long int mean_time;
int main()
{
 	// generate data
	short data[SIZE];
	for (int i = 0; i < SIZE; i++) {
		data[i] = rand() % 512;
	}

    // printf("With sort\n");
    // qsort(data, SIZE, sizeof(data[0]), compare);
	long long sum = 0;
    
	for (int j = 0; j < 10000; j++) {
        
		for (int i = 0; i < SIZE; i++) {
            clock_gettime(CLOCK_MONOTONIC, &t1);
			if (data[i] >= 256) {
				sum += data[i];
			}
            clock_gettime(CLOCK_MONOTONIC, &t2);
            mean_time += (long long)(t2.tv_sec - t1.tv_sec) * 1000000000 + t2.tv_nsec - t1.tv_nsec;
		}
	}
    long long int delta_t = (long long)(t2.tv_sec - t1.tv_sec) * 1000000000
                          + t2.tv_nsec - t1.tv_nsec;
    printf ("elapsed time: %lld [ns]\n", delta_t);
    printf("mean time : %lld [ns]\n", (mean_time/SIZE));
	printf ("sum=%lld\n", sum);
}