/**
 * Copyright 2018 University of Applied Sciences Western Switzerland / Fribourg
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Project:	HEIA-FR / HES-SO MSE - MA-CSEL1 Laboratory
 *
 * Abstract: System programming -  file system
 *
 * Purpose:	NanoPi silly status led control system
 *
 * Autĥor:	Daniel Gachet
 * Date:	07.11.2018
 */
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


static int open_led()
{
    // unexport pin out of sysfs (reinitialization)
    int f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, LED, strlen(LED));
    close(f);

    // export pin to sysfs
    f = open(GPIO_EXPORT, O_WRONLY);
    write(f, LED, strlen(LED));
    close(f);

    // config pin
    f = open(GPIO_LED "/direction", O_WRONLY);
    write(f, "out", 3);
    close(f);

    // open gpio value attribute
    f = open(GPIO_LED "/value", O_RDWR);
    return f;
}

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

    f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, SW_K3, strlen(SW_K3));
    close(f);

    f = open(GPIO_EXPORT,O_WRONLY);
    write(f, SW_K3, strlen(SW_K3));
    close(f);

    // config pin
    f = open(SW_K3 "/direction", O_WRONLY);
    write(f, "in", 2);
    close(f);
    return f;

}


int main(int argc, char* argv[])
{
    long duty   = 2;     // %
    long period = 1000;  // ms
    if (argc >= 2) period = atoi(argv[1]);
    period *= 1000000;  // in ns

    // compute duty period...
    long p1 = period / 100 * duty;
    long p2 = period - p1;

    int led = open_led();
    pwrite(led, "1", sizeof("1"), 0);

    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);

    // ----------- multiplex  ----------- 
    open_switchs();

    int f_k1 = open("/sys/class/gpio/gpio0/value",O_RDONLY);
    int f_k2 = open("/sys/class/gpio/gpio2/value",O_RDONLY);
    if (f_k1 == -1) {
        printf("file not found!\n");
    } 

    printf("fd k1 : %d | fd k2 : %d\n",f_k1,f_k2);

    int epfd = epoll_create1(0);

    if (epfd == -1){
        printf("ERROR : epoll create %d\n",epfd);
        exit(1);
    }
        

    struct epoll_event event = {
        .events =   EPOLLET,
        .data.fd = f_k1,
    };

   int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, f_k1, &event);
   if (ret == -1){
       printf("ERROR : epoll ctl %d\n", ret);
       exit(1);
   }        

    // struct epoll_event events[2];
    // events[0].events = EPOLLET;
    // events[0].data.fd = f_k1;

    // events[1].events = EPOLLET;
    // events[1].data.fd = f_k2;


    int k = 0;
    while (1) {
        int nr = epoll_wait(epfd, &event, 2, -1);
        if (nr == -1)
            printf("ERROR : epoll wait\n");
        printf("wait");
    for (int i=0; i<nr; i++) {
            // printf ("event=%ld on fd=%d\n", events[i].events, events[i].data.fd);
            printf ("event=%ld on fd=%d\n", event.events, event.data.fd);
            char buffer[10];
            read(event.data.fd, buffer,2);
            printf("read : %s\n",buffer);
            // operation on events[i].data.fd can be performed without blocking... 
        }

        struct timespec t2;
        clock_gettime(CLOCK_MONOTONIC, &t2);

        long delta =
            (t2.tv_sec - t1.tv_sec) * 1000000000 + (t2.tv_nsec - t1.tv_nsec);

        int toggle = ((k == 0) && (delta >= p1)) | ((k == 1) && (delta >= p2));
        if (toggle) {
            t1 = t2;
            k  = (k + 1) % 2;
            if (k == 0)
                pwrite(led, "1", sizeof("1"), 0);
            else
                pwrite(led, "0", sizeof("0"), 0);
        }
    }

    return 0;
}
