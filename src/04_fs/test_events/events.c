#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <stdio.h>
#include <sys/timerfd.h>


/*
 * status led - gpioa.10 --> gpio10
 * power led  - gpiol.10 --> gpio362
 */
#define STEP 1
#define MAX_VAL 20

#define GPIO_EXPORT   "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"

const char* GPIO_SWITCHS[] = {"/sys/class/gpio/gpio0", "/sys/class/gpio/gpio2", "/sys/class/gpio/gpio3"};

const char* SW[] = {"0", "2", "3"};

void write_attr(char *attr, char *value){
    // build attribute path in sysfs
    char path[80] = "/sys/class/fan_management_class/fan_management/";
    strcat(path, attr);

    // open /sys file
    int fd_driver = open(path, O_WRONLY);
        if (fd_driver == -1)
            printf("error : %d, opening driver attribute\n",errno);
            // syslog(LOG_INFO,
            //     "error : %d, opening driver attribute\n",
            //     errno);
    
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
            printf("error opening file %d\n",fd_driver);
            // syslog(LOG_INFO,
            //     "error : %d, opening driver attribute\n",
            //     errno);
    
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
    // ----------- multiplex  ----------- 
    open_switchs();

    int fd_switchs[3];
    fd_switchs[0] = open("/sys/class/gpio/gpio0/value",O_RDONLY);
    fd_switchs[1] = open("/sys/class/gpio/gpio2/value",O_RDONLY);
    fd_switchs[2] = open("/sys/class/gpio/gpio3/value",O_RDONLY);

    int epfd = epoll_create1(0);

    if (epfd == -1){
        printf("ERROR : epoll create %d\n",epfd);
        exit(1);
    }
    printf("epoll created\n");

    for(int i=0;i<3;i++){
    struct epoll_event event_switch[3];

    if (fd_switchs[i] == -1) {
        printf("file k%d not found!\n",(i+1));
        exit(1);
    } 
    printf("fd k%d : %d |",(i+1),fd_switchs[i]);

    event_switch[i].events = EPOLLET;
    event_switch[i].data.fd = fd_switchs[i];

    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd_switchs[i], &event_switch[i]);
    if (ret == -1){
        printf("ERROR : epoll ctl %d\n", ret);
        exit(1);
    }    
}

#if 1
    char buffer[3];
    struct epoll_event events[5];
    while (1)
    {
        int nr = epoll_wait(epfd, events, 5, -1);
        if (nr == -1)
            printf("ERROR : epoll wait\n");

        printf("number of event : %d\n",nr);
        for (int i=0; i<nr; i++) {
            if(events[i].data.fd == fd_switchs[0]){
                
                read_attr("frequency",buffer);
                int frequency = atoi(buffer);
                frequency = (frequency + STEP) % (MAX_VAL+1);
                sprintf(buffer, "%d", frequency); 
                write_attr("frequency",buffer);
                printf("increase fan speed : new frequency %s\n",buffer);
            }   
            else if(events[i].data.fd == fd_switchs[1]){
                
                read_attr("frequency",buffer);

                unsigned char frequency = atoi(buffer);
                frequency = (frequency - STEP) % (MAX_VAL+1);
                sprintf(buffer, "%d", frequency); 
                
                write_attr("frequency",buffer);
                printf("decrease fan speed : new frequency %s\n",buffer);

            }
            else if(events[i].data.fd == fd_switchs[2]){
                // char buffer[3];
                read_attr("mode",buffer);
                if(buffer[0] == 49 ){
                    write_attr("mode","0");
                }
                else if(buffer[0] == 48){
                    write_attr("mode","1");
                }
                printf("change mode\n");
            }
        }
    }
#endif   
    
    return 0;
}