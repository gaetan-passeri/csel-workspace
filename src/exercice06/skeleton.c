// skeleton.c
#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging

#include <linux/moduleparam.h>	// needed for module parameters
#include <linux/kthread.h>
#include <linux/delay.h>

struct task_struct* k;

static int thread(void* data){
    while (!kthread_should_stop())
    {
        pr_info("See you in 5 sec");
        ssleep(5);
    }
    return 0;
}
static int __init skeleton_init(void)
{
	pr_info ("Linux module 06 loaded\n");
	k = kthread_run(thread, NULL, "EX06");
	return 0;
}

static void __exit skeleton_exit(void)
{
    kthread_stop(k);
	pr_info ("Linux module skeleton unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION ("Module skeleton");
MODULE_LICENSE ("GPL");
