// skeleton.c
#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging

#include <linux/moduleparam.h>	// needed for module parameters
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/wait.h>

struct task_struct* thread_1; 
struct task_struct* thread_2;

atomic_t start;

DECLARE_WAIT_QUEUE_HEAD(queue);

static int thread(void* data){

    pr_info("Thread 1 is active");
    while (!kthread_should_stop())
    {
        pr_info("Thread 1: is waiting for thread 2");
        wait_event(queue, atomic_read(&start) != 0 || kthread_should_stop());
        
        pr_info("Thread 1: is active");
        atomic_set(&start,0);
    }
    return 0;
}

static int thread2(void* data){

    wait_queue_head_t queue_2;
    init_waitqueue_head(&queue_2);

    pr_info("Thread 2: is active");
    while (!kthread_should_stop())
    {
        int ret = wait_event_timeout(queue, kthread_should_stop(), 5*HZ);
        if(ret == 0){
            pr_info("Thread 2: Timeout elapsed");
        }
        atomic_set(&start,1);
        wake_up(&queue);
        pr_info("Thread 2: See you in 5 sec");
    }
    return 0;
}
static int __init skeleton_init(void)
{
	pr_info ("Linux module 07 loaded\n");
    atomic_set(&start,1);
    thread_2 = kthread_run(thread2, NULL, "EX07 thread 2");
	thread_1 = kthread_run(thread, NULL, "EX07 thread 1");
	return 0;
}

static void __exit skeleton_exit(void)
{
    kthread_stop(thread_1);
    kthread_stop(thread_2);
	pr_info ("Linux module 07 skeleton unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Glenn Muller <glenn.mullerar@hes-so.ch>");
MODULE_DESCRIPTION ("Module Exercice07");
MODULE_LICENSE ("GPL");
