#define _GNU_SOURCE     // attribution des coeurs cpu aux processus
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>     // for read and write sys. calls
#include <errno.h>      // gestion des messages d'erreurs
#include <signal.h>     // interception des signaux
#include <sched.h>      // thread
#include <sys/types.h>  // pour l'utilisation de socketpair
#include <sys/socket.h> // ""

int fd[2]; // files descriptor socketpair between parent and child (arbitrary : fd[0] = parent, fd[1] = child)

int parent_pid;
int child_pid;

// fonction appelée lorsqu'un signal SIGINT est intercepté => quitte l'application
void exit_app(){
    printf("\nexit app\n");
    exit(EXIT_SUCCESS);
}

// fonction executée en boucle par le thread enfant => écoute les messages de l'utilisateur et les transmets via socketpair au thread principal
void child_routine(){
    char buf[32];
    child_pid = getpid();

    while(1) {
        // get str from user's keyboard
        fgets(buf, 32, stdin);
        
        // test
        printf("received msg in child (pid %d) : %s", getpid(), buf);

        // write str to parent processus
        ssize_t count = write (fd[0], buf, strlen(buf));
        if (count == -1)
            perror("\nerror");        
    }
}

void parent_routine(){
    char buf[32];
    parent_pid = getpid();
    int res;
    size_t len;
    ssize_t nr;

    while(1) {
        len = sizeof(buf);
        nr = read (fd[1], buf, len);
        if (nr == -1)
            perror("parent reading");

        // test
        printf("\nreceived msg in parent (pid %d) : %s", getpid(), buf);        

        printf("\nbuf : %s", buf);        

        res = strcmp(buf, "exit");
        printf("\nstrcomp result : %d", res);
        if(res == 0){
            printf("\nkill that bitch");
            // kill child process
            int ret = kill(child_pid, SIGKILL);
            if (ret == -1) {
                perror("kill");
                exit(EXIT_FAILURE);
            }

            // exit main process
            exit(EXIT_SUCCESS);
        }
    }
}

int main(int argc, char* argv[])
{
    // capture du signal SIGINT (Ctrl-C)
    struct sigaction act = {.sa_handler = exit_app,}; // replace exit_app with NULL pointer
    int err = sigaction (SIGINT, &act, NULL);
    if(err == -1)
        perror("capture SIGINT signal");

    // création socketpair
    err = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
    if (err == -1)
        perror("socketpair creation");
    
    pid_t pid = fork();

    if (pid == 0) {
        printf("child pid : %d", pid);
        // ---- attribution des coeurs aux processus ----------------------------------------------
        cpu_set_t set_child;
        CPU_ZERO(&set_child);       // rst du set de cpu enfant (désattribue tous les éventuels cpu de ce set)
        CPU_SET(1, &set_child);    // ajout du coeur 1 au set de cpu enfant
        int ret = sched_setaffinity(pid, sizeof(set_child), &set_child);    // attribution du set de cpu enfant au thread enfant
        if(ret == -1)
            perror("child cpu set creation");
        // ----------------------------------------------------------------------------------------
        child_routine();
    } else if (pid > 0){
        printf("parent pid : %d", pid);
        // ---- attribution des coeurs aux processus ----------------------------------------------
        cpu_set_t set_parent;
        CPU_ZERO(&set_parent);       // rst du set de cpu parent (désattribue tous les éventuels cpu de ce set)
        CPU_SET(0, &set_parent);    // ajout du coeur 0 au set de cpu parent
        int ret = sched_setaffinity(pid, sizeof(set_parent), &set_parent);    // attribution du set de cpu parent au thread courant (NB : 0 correspond au thread courant)
        if(ret == -1)
            perror("parent cpu set creation");
        // ----------------------------------------------------------------------------------------
        parent_routine();
    } else {
        perror("fork pid return");
    }

    return 0;
}
