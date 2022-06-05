#include <stdlib.h>
#include <stdio.h>
#include <errno.h>      // gestion des messages d'erreurs

#define SIZE_IN_BYTES 30000000

int main(int argc, char* argv[])
{
    // allocate 20 MB of memory
    char *ptr;
    ptr = malloc(SIZE_IN_BYTES);
    if(ptr == NULL) {
        perror("memory allocation");
        exit(EXIT_FAILURE);
    }

    // fill memory with zeroes
    for(int i=0; i<SIZE_IN_BYTES; i++){
        ptr[i] = 0;
    }

    // print memory content
    for(int i=0; i<SIZE_IN_BYTES; i++){
        printf("%d",ptr[i]);
    }

    // free memory
    free(ptr);
    ptr = NULL;
    
    return 0;
}