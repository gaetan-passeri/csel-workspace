#define _GNU_SOURCE     // attribution des coeurs cpu aux processus
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>     // for read and write sys. calls
#include <fcntl.h>      // for open flags
#include <errno.h>      // gestion des messages d'erreurs
#include <signal.h>     // interception des signaux
#include <sched.h>      // thread
#include <sys/types.h>  // pour l'utilisation de socketpair
#include <sys/socket.h> // ""

// fonction appelée lorsqu'un signal SIGINT est intercepté => quitte l'application
void exit_app(){
    printf("\nexit fan IPC control interface\n");
    exit(EXIT_SUCCESS);
}

void send_cfg_to_fifo(char *cmd_line){
    // open fifo file
    int fd = open ("/tmp/myfifo", O_WRONLY);
    if (fd == -1)
        perror("error opening fifo file");

    // send msg to daemon via fifo
    write(fd, cmd_line, strlen(cmd_line));

    // close fifo file
    close(fd);
}

int main(int argc, char* argv[])
{
    char buf[100]; // user keyboard scan buffer

    // subchar ptr to manage commands
    char *cmd;
    char *value;
    char line_cmd[32];
    
    const char *welcome_msg = "\n \
    Welcome to the fan IPC control interface !\n \
    Enter 'help' to display available commands.\n";

    const char *msg_help = "\n \
    help                        // display this message\n \
    set_mode [mode]             // [mode] = AUTO or MAN\n \
    set_frequency [frequency]   // [frequency] = integer between 1 and 100\n \
    exit                        // close application\n";
    
    // capture du signal SIGINT (Ctrl-C)
    struct sigaction act = {.sa_handler = exit_app,}; // replace exit_app with NULL pointer
    int err = sigaction (SIGINT, &act, NULL);
    if(err == -1)
        perror("capture SIGINT signal");

    // display welcome message
    printf("%s", welcome_msg);

    while(1) {
        printf("\n> ");
        // get str from user's keyboard
        fgets(buf, 32, stdin);
        // close app
        if(strcmp(buf, "exit\n")==0){
            // send_cfg_to_fifo("exit"); // to simplifying tests
            exit_app();
        }
        else if(strcmp(buf, "help\n")==0){
            // display help
            printf("%s", msg_help);
        }else{
            // manage setting commands
            cmd = strtok (buf, " ");
            if(cmd != NULL){
                value = strtok(NULL, " ");
            }
            value[strlen(value)-1] = 0;
            if((strcmp(cmd, "set_mode") == 0) && ((strcmp(value, "AUTO") == 0) | (strcmp(value, "MAN") == 0))) {
                // build cmd line
                if(strcmp(value, "AUTO") == 0){
                    sprintf(line_cmd, "mode %d", 0);
                } else {
                    sprintf(line_cmd, "mode %d", 1);
                }
                // write cmd line to FIFO
                send_cfg_to_fifo(line_cmd);
            }else if (strcmp(cmd, "set_frequency") == 0 && atoi(value) >= 1 && atoi(value) <= 100)
            {
                sprintf(line_cmd, "frequency %d", atoi(value));
                send_cfg_to_fifo(line_cmd);
            }else {
                printf("unreconized command, enter 'help' to display commands list\n");
            }    
        }        
    }

    return 0;
}
