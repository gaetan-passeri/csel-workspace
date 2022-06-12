// skeleton.c
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging
#include <linux/module.h>	// needed by all modules
#include <linux/timer.h>	// need for timer
#include <linux/thermal.h>	// need for get cpu temp 
#include <linux/gpio.h>		// need for access GPIIO
#include <linux/moduleparam.h>	// needed for module parameters

enum Mode {AUTO, MAN};
static int current_mode = AUTO;
module_param(current_mode, int, 0);
static int  frequency_Hz = 2;		// 2, 5, 10, 20
module_param(frequency_Hz, int, 0);

const char *cpu_thermal = "cpu-thermal";
static struct timer_list my_timer;

static void timer_callback(struct timer_list *timer){
	int ret, temp;
	
	// printk("%s called (%ld)\n", __func__, jiffies);	

	// get and print cpu temp
	struct thermal_zone_device* thermal_zone = thermal_zone_get_zone_by_name (cpu_thermal);
	thermal_zone_get_temp(thermal_zone, &temp);
	pr_info("CPU temperature : %d",temp);
	
	// set next timer interval and restart
	ret = mod_timer(&my_timer, jiffies + msecs_to_jiffies(2000));
	if (ret)
		pr_err("%s: Timer firing failed\n", __func__);
}

static int __init skeleton_init(void)
{
	int ret;
	pr_info ("Linux module fan management loaded\n");

	// set timer
	timer_setup(&my_timer, timer_callback, CLOCK_MONOTONIC);
	pr_info("%s: Setup timer to fire in 2s (%ld)\n", __func__, jiffies);

	// set timer interval and start
	ret = mod_timer(&my_timer, jiffies + msecs_to_jiffies(2000));
	if (ret)
		pr_err("%s: Timer firing failed\n", __func__);
	printk("Timer Started\r\n");
	return 0;
}

static void __exit skeleton_exit(void)
{
	int ret;
	pr_info ("Linux module fan management unloaded\n");

	// delete timer from list
	ret = del_timer(&my_timer);
	if(ret)
		pr_err("%s: The timer is still is use ...\n", __func__);
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Glenn Muller <glenn.mullerar@hes-so.ch>");
MODULE_DESCRIPTION ("Linux module fan management");
MODULE_LICENSE ("GPL");
