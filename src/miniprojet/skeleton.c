// skeleton.c
#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging
#include <linux/thermal.h>	// need for get cpu temp 
#include <linux/gpio.h>		// need for access GPIIO
#include <linux/timer.h>	// need for timer
#include <linux/moduleparam.h>	// needed for module parameters

enum Mode {AUTO, MAN};
static int current_mode = AUTO;
module_param(current_mode, int, 0);
static int  frequency_Hz = 2;		// 2, 5, 10, 20
module_param(frequency_Hz, int, 0);

const char *cpu_thermal = "cpu-thermal";

struct timer_list mytimer;

void timer_callback(void){
	struct thermal_zone_device* thermal_zone = thermal_zone_get_zone_by_name (cpu_thermal);
	int temp;	

	thermal_zone_get_temp(thermal_zone, &temp);

	pr_info("CPU temperature : %d",temp);

	//add_timer(&mytimer); // restart timer
}

static int __init skeleton_init(void)
{
	pr_info ("Linux module fan management loaded\n");

	// set timer frequency
	timer_setup(&mytimer, (void *) timer_callback, CLOCK_MONOTONIC);
	mod_timer(&mytimer, 1000000000);
	add_timer(&mytimer);
	
	printk("Timer Started\r\n");
	//timer_setup(timer, timer_callback, TIMER_MONOTONIC);

	return 0;
}

static void __exit skeleton_exit(void)
{
	pr_info ("Linux module fan management unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Glenn Muller <glenn.mullerar@hes-so.ch>");
MODULE_DESCRIPTION ("Linux module fan management");
MODULE_LICENSE ("GPL");
