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
#define GPIO_EXPORT   "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define GPIO_LED      "/sys/class/gpio/gpio10"
#define GPIO_K1      "/sys/class/gpio/gpio0"
#define GPIO_K2      "/sys/class/gpio/gpio2"
#define GPIO_K3      "/sys/class/gpio/gpio3"
#define LED           "10"
#define SW_K1         "0"
#define SW_K2         "2"
#define SW_K3         "3"

const char* GPIO_SWITCHS[] = {"/sys/class/gpio/gpio0", "/sys/class/gpio/gpio2", "/sys/class/gpio/gpio3"};

const char* SW[] = {"0", "2", "3"};

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
    
#if 1

    int epfd = epoll_create1(0);

    if (epfd == -1){
        printf("ERROR : epoll create %d\n",epfd);
        exit(1);
    }
    printf("epoll created\n");

    // struct epoll_event event_k2 = {
    //     .events = EPOLLET,
    //     .data.fd = f_k2,
    // };

    // struct epoll_event event_k3 = {
    //     .events = EPOLLET,
    //     .data.fd = f_k3,
    // };

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

#endif

#if 0
    printf("events done \n");
    
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, f_k1, &event_k1);
    if (ret == -1){
        printf("ERROR : epoll ctl %d\n", ret);
        exit(1);
    }    
    
    printf("event add to epoll k1\n");
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, f_k2, &event_k2);
    if (ret == -1){
        printf("ERROR : epoll ctl %d\n", ret);
        exit(1);
    }   
    
    printf("event add to epoll k2\n");
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, f_k3, &event_k3);
    if (ret == -1){
        printf("ERROR : epoll ctl %d\n", ret);
        exit(1);
    }   

    printf("event add to epoll k3\n");
#endif

#if 1
    struct epoll_event events[5];
    while (1)
    {
        int nr = epoll_wait(epfd, events, 5, -1);
        if (nr == -1)
            printf("ERROR : epoll wait\n");

        printf("number of event : %d\n",nr);
        for (int i=0; i<nr; i++) {
            if(events[i].data.fd == fd_switchs[0]){
                printf("increase fan speed\n");
            }
            else if(events[i].data.fd == fd_switchs[1]){
                printf("decrease fans speed\n");
            }
            else if(events[i].data.fd == fd_switchs[2]){
                printf("change mode \n");
            }
        }
    }
#endif   
    
    return 0;
}