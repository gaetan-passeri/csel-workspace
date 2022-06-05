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


/*
 * status led - gpioa.10 --> gpio10
 * power led  - gpiol.10 --> gpio362
 */
#define GPIO_EXPORT   "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define GPIO_LED      "/sys/class/gpio/gpio10"
#define LED           "10"
#define SW_K1         "0"
#define SW_K2         "2"
#define SW_K3         "3"
int yes = 1;


static int open_switchs(){
    int f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, SW_K1, strlen(SW_K1));
    close(f);

    f = open(GPIO_EXPORT,O_WRONLY);
    write(f, SW_K1, strlen(SW_K1));
    close(f);

    // config pin
    f = open(SW_K1 "/direction", O_WRONLY);
    write(f, "in", 2);
    close(f);


    f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, SW_K2, strlen(SW_K2));
    close(f);

    f = open(GPIO_EXPORT,O_WRONLY);
    write(f, SW_K2, strlen(SW_K2));
    close(f);

    // config pin
    f = open(SW_K2 "/direction", O_WRONLY);
    write(f, "in", 2);
    close(f);

    return f;
}


int main(int argc, char* argv[])
{
    printf("Hello main\n");
    // ----------- multiplex  ----------- 
    open_switchs();

    int f_k1 = open("/sys/class/gpio/gpio0/value",O_RDONLY);
    int f_k2 = open("/sys/class/gpio/gpio2/value",O_RDONLY);
    
    if (f_k1 == -1) {
        printf("file k1 not found!\n");
        exit(1);
    } 
    if(f_k2 == -1){
        printf("file k2 not found\n");
        exit(1);
    }

    printf("fd k1 : %d | fd k2 : %d\n",f_k1,f_k2);

    int epfd = epoll_create1(0);

    if (epfd == -1){
        printf("ERROR : epoll create %d\n",epfd);
        exit(1);
    }

    struct epoll_event event = {
        .events = EPOLLIN | EPOLLET,// EPOLLET,
        .data.fd = f_k1,
    };

    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, f_k1, &event);
    if (ret == -1){
        printf("ERROR : epoll ctl %d\n", ret);
        exit(1);
    }    

    while (1)
    {
        int nr = epoll_wait(epfd, &event, 2, -1);
        if (nr == -1)
            printf("ERROR : epoll wait\n");
        printf("wait\n");


        for (int i=0; i<nr; i++) {
            // printf ("event=%ld on fd=%d\n", events[i].events, events[i].data.fd);
            printf("event number : %d\n",nr);

            printf ("event=%ld on fd=%d\n", event.events, event.data.fd);
            char buffer;
            read(event.data.fd, &buffer,1);
            printf("read : %c\n",buffer);
            // operation on events[i].data.fd can be performed without blocking... 
        }
        printf("end for loop\n");
    }
    
    return 0;
}