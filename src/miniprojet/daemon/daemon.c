/**
 * Copyright 2015 University of Applied Sciences Western Switzerland / Fribourg
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Project:	HEIA-FR / Embedded Systems Laboratory
 *
 * Abstract: Process and daemon samples
 *
 * Purpose: This module implements a simple daemon to be launched
 *          from a /etc/init.d/S??_* script
 *          -> this application requires /opt/daemon as root directory
 *
 * Autĥor:  Daniel Gachet
 * Date:    17.11.2015
 */
#define _XOPEN_SOURCE 600
#define _DEFAULT_SOURCE

#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>

#define UNUSED(x) (void)(x)
#define GPIO_EXPORT   "/sys/class/gpio/export"          
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"

const char* GPIO_SWITCHS[] = {"/sys/class/gpio/gpio0", "/sys/class/gpio/gpio2", "/sys/class/gpio/gpio3"};

const char* SW[] = {"0", "2", "3"};     // pin number of each switch

#define STEP 1                          // step to increase or decrease frequency
#define MAX_FREQ 20                      // Max frequency

static int signal_catched = 0;
char fifo_buf[100];
char *cmd;
char *value;

static void catch_signal(int signal)
{
    syslog(LOG_INFO, "signal=%d catched\n", signal);
    signal_catched++;
}

static void fork_process()
{
    pid_t pid = fork();
    switch (pid) {
        case 0:
            break;  // child process has been created
        case -1:
            syslog(LOG_ERR, "ERROR while forking");
            exit(1);
            break;
        default:
            exit(0);  // exit parent process with success
    }
}

void write_attr(char *attr, char *value){
    // build attribute path in sysfs
    char path[80] = "/sys/class/fan_management_class/fan_management/";
    strcat(path, attr);

    // open /sys file
    int fd_driver = open(path, O_WRONLY);
        if (fd_driver == -1)
            syslog(LOG_INFO,
                "error : %d, opening driver attribute\n",
                errno);
    
    // write attribute value
    write(fd_driver, value, strlen(value));

    close(fd_driver);
}

void read_attr(char* attr, char* value){
    char path[80] = "/sys/class/fan_management_class/fan_management/";
    strcat(path, attr);

    // open /sys file
    int fd_driver = open(path, O_RDONLY);
        if (fd_driver == -1)
            syslog(LOG_INFO,
                "error : %d, opening driver attribute\n",
                errno);
    
    // read attribute value
    read(fd_driver, value, strlen(value));
    close(fd_driver);
}
static void open_switchs(){
    int f;
    
    for(int i=0;i<3;i++){
        char direction[31];
        char edge[26];
        
        // creat string to config GPIO
        strcpy(direction,GPIO_SWITCHS[i]);
        strcat(direction,"/direction");

        strcpy(edge,GPIO_SWITCHS[i]);
        strcat(edge,"/edge");

        // reset previouse config
        f = open(GPIO_UNEXPORT, O_WRONLY);
        write(f, SW[i], strlen(SW[i]));
        close(f);

        // export GPIO
        f = open(GPIO_EXPORT,O_WRONLY);
        write(f, SW[i], strlen((SW[i])));
        close(f);

        // config pin as INPUT
        f = open(direction, O_WRONLY);
        write(f, "in", 2);
        close(f);

        // config pin for rising event 
        f = open(edge, O_WRONLY);
        write(f, "rising", 6);
        close(f);

    }
}

int main(int argc, char* argv[])
{
    int fd_fifo; // to use fifo file
    int fifo_msg_len;
    
    UNUSED(argc);
    UNUSED(argv);

    // 1. fork off the parent process
    fork_process();

    // 2. create new session
    if (setsid() == -1) {
        syslog(LOG_ERR, "ERROR while creating new session");
        exit(1);
    }

    // 3. fork again to get rid of session leading process
    fork_process();

    // 4. capture all required signals
    struct sigaction act = {
        .sa_handler = catch_signal,
    };
    sigaction(SIGHUP, &act, NULL);   //  1 - hangup
    sigaction(SIGINT, &act, NULL);   //  2 - terminal interrupt -- enable for testing
    sigaction(SIGQUIT, &act, NULL);  //  3 - terminal quit
    sigaction(SIGABRT, &act, NULL);  //  6 - abort
    sigaction(SIGTERM, &act, NULL);  // 15 - termination
    sigaction(SIGTSTP, &act, NULL);  // 19 - terminal stop signal

    // 5. update file mode creation mask
    umask(0027);

    // 6. change working directory to appropriate place
    if (chdir("/") == -1) {
        syslog(LOG_ERR, "ERROR while changing to working directory");
        exit(1);
    }

    // 7. close all open file descriptors
    for (int fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--) {
        close(fd);
    }

    // 8. redirect stdin, stdout and stderr to /dev/null
    if (open("/dev/null", O_RDWR) != STDIN_FILENO) {
        syslog(LOG_ERR, "ERROR while opening '/dev/null' for stdin");
        exit(1);
    }
    if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO) {
        syslog(LOG_ERR, "ERROR while opening '/dev/null' for stdout");
        exit(1);
    }
    if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO) {
        syslog(LOG_ERR, "ERROR while opening '/dev/null' for stderr");
        exit(1);
    }

    // 9. option: open syslog for message logging
    openlog(NULL, LOG_NDELAY | LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "Daemon has started...");

    // 10. option: get effective user and group id for appropriate's one
    struct passwd* pwd = getpwnam("daemon");
    if (pwd == 0) {
        syslog(LOG_ERR, "ERROR while reading daemon password file entry");
        exit(1);
    }

    // 11. option: change root directory
    if (chroot(".") == -1) {
        syslog(LOG_ERR, "ERROR while changing to new root directory");
        exit(1);
    }

    // the step 12 is not done because root right is needed to open fifo file.
    // 12. option: change effective user and group id for appropriate's one
    // if (setegid(pwd->pw_gid) == -1) {
    //     syslog(LOG_ERR, "ERROR while setting new effective group id");
    //     exit(1);
    // }
    // if (seteuid(pwd->pw_uid) == -1) {
    //     syslog(LOG_ERR, "ERROR while setting new effective user id");
    //     exit(1);
    // }

    // 13. implement daemon body...
    int pid = getpid();
    syslog(LOG_INFO,"fifo file created\n");
    syslog(LOG_INFO, "daemon's PID : %d\n", pid); 
    // create named pipe FIFO file in /opt/
    char * myfifo = "myfifo";
    int res = mkfifo("/tmp/myfifo", 0666);
    syslog(LOG_INFO,"fifo file created\n");
    if(res == -1){ 
        syslog(LOG_INFO,
        "error %d creating fifo file\n",
        errno);
    }
    // open fifo file
    fd_fifo = open ("/tmp/myfifo", O_RDWR);
    if (fd_fifo == -1){
        syslog(LOG_INFO,
           "error : %d, opening fifo file in %s\n",
           errno, myfifo);
    }
    open_switchs();

    syslog(LOG_INFO,"switches configured\n");

    int fd_switchs[3];
    fd_switchs[0] = open("/sys/class/gpio/gpio0/value",O_RDONLY);
    fd_switchs[1] = open("/sys/class/gpio/gpio2/value",O_RDONLY);
    fd_switchs[2] = open("/sys/class/gpio/gpio3/value",O_RDONLY);

    int epfd = epoll_create1(0);

    if (epfd == -1){
        syslog(LOG_INFO,"ERROR : epoll create %d\n",epfd);
        exit(1);
    }
    printf("epoll created\n");

    for(int i=0;i<3;i++){
        struct epoll_event event_switch[3];

        if (fd_switchs[i] == -1) {
            syslog(LOG_INFO,"file k%d not found!\n",(i+1));
            exit(1);
        } 
        syslog(LOG_INFO,"fd k%d : %d \n",(i+1),fd_switchs[i]);

        event_switch[i].events = EPOLLET;
        event_switch[i].data.fd = fd_switchs[i];

        int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd_switchs[i], &event_switch[i]);
        if (ret == -1){
            syslog(LOG_INFO,"ERROR : epoll ctl %d\n", ret);
            exit(1);
        }
    }

    struct epoll_event events[5];
    pid_t pid_epoll = fork();
    int nr;
    char buffer[3];
    unsigned int frequency;
    while(1) {
        // get fifo messages
        switch (pid_epoll)
        {
        case 0:
            nr = epoll_wait(epfd, events, 5, -1);
            if (nr == -1)
                syslog(LOG_INFO,"ERROR : epoll wait\n");

            printf("number of event : %d\n",nr);
            for (int i=0; i<nr; i++) {
                if(events[i].data.fd == fd_switchs[0]){
                    read_attr("frequency",buffer);
                    
                    frequency = atoi(buffer);
                    frequency = (frequency + STEP) % (MAX_FREQ+1);
                    
                    sprintf(buffer, "%d", frequency); 
                    write_attr("frequency",buffer);
                    
                    syslog(LOG_INFO,"increase fan speed : %s [Hz]\n",buffer);
                }
                else if(events[i].data.fd == fd_switchs[1]){
                    read_attr("frequency",buffer);

                    frequency = atoi(buffer);
                    frequency = (frequency - STEP) % (MAX_FREQ+1);
                    sprintf(buffer, "%d", frequency); 
                    
                    write_attr("frequency",buffer);
                    syslog(LOG_INFO,"decrease fans speed : %s [Hz]\n",buffer);
                }
                else if(events[i].data.fd == fd_switchs[2]){
                    
                    read_attr("mode",buffer);
                    if(buffer[0] == 49 ){
                        write_attr("mode","0");
                    }
                    else if(buffer[0] == 48){
                        write_attr("mode","1");
                    }
                    syslog(LOG_INFO,"change mode \n");
                }
            }
            break;
        case -1 : 
            syslog(LOG_ERR, "ERROR while forking");
            exit(1);
            break;
        default:
            fifo_msg_len = read(fd_fifo, fifo_buf, sizeof(fifo_buf));
            if(fifo_msg_len > 0){
                fifo_buf[fifo_msg_len] = 0;
                syslog(LOG_INFO, "fifo received msg : %s\n", fifo_buf);

                if(strcmp(fifo_buf, "exit") == 0) {
                    break;
                } else{
                    syslog(LOG_INFO, "test après if exit\n");

                    cmd = strtok(fifo_buf, " ");
                    if(cmd != NULL){
                        value = strtok(NULL, " ");
                    }
                }

                syslog(LOG_INFO, "test après strtok\n");

                syslog(LOG_INFO, "test cmd : %s\n", cmd);
                
                syslog(LOG_INFO, "cmd : %s, value %s\n", cmd, value);

                write_attr(cmd, value);
            }
            break;
        }
    }
    // close fifo file
    close(fd_fifo);

    syslog(LOG_INFO,
           "daemon stopped. Number of signals catched=%d\n",
           signal_catched);
    closelog();

    return 0;
}