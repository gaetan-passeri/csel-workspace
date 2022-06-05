#define _GNU_SOURCE     // attribution des coeurs cpu aux processus
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>      // gestion des messages d'erreurs
#include <sched.h>      // thread
#include <unistd.h>     // syscall fork

int main(int argc, char* argv[])
{
    // create a second process with fork
    pid_t pid = fork();

    if (pid == 0) {
        int a = 0;
        while(1) {
            a++;
        }
    } else if (pid > 0){
        int b = 0;
        while(1) {
            b++;
        }
    } else {
        perror("fork pid return");
    }
    
    return 0;
}