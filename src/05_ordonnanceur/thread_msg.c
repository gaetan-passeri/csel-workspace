#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

int exit_app(){
    printf("\nexit app");
    exit(0);
}

int main(int argc, char* argv[])
{
    //int err = sigaction (SIGHUP, &exit_app, NULL);

    if(argc < 2){
        printf("\nError, no argument !");
        return 1;
    } else {
        printf("\nHello ma gueuze!\nprog. name : %s\nargument : %s\n", argv[0], argv[1]);
    }

    // empty loop -> waiting exit cmd
    while(1) {

    }

    return 0;
}
