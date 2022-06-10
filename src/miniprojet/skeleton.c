// skeleton.c
#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging
#include <linux/thermal.h>	// need for get cpu temp 
#include <linux/gpio.h>		// need for access GPIIO
#include <linux/timer.h>	// need for timer
#include <linux/moduleparam.h>	// needed for module parameters

static char* text = "dummy text";
module_param(text, charp, 0664);
static int  elements = 1;
module_param(elements, int, 0);

const char * cpu_thermal = "cpu-thermal ";

static int __init skeleton_init(void)
{
	pr_info ("Linux module fan management loaded\n");
	//pr_debug ("  text: %s\n  elements: %d\n", text, elements);
	pr_debug("sjkdas");
	struct thermal_zone_device* thermal_zone = thermal_zone_get_zone_by_name (cpu_thermal);
	int temp;
	
	thermal_zone_get_temp(thermal_zone, &temp);

	pr_info("CPU temperature : %d",temp);

	return 0;
}

static void __exit skeleton_exit(void)
{
	pr_info ("Linux module skeleton unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Glenn Muller <glenn.mullerar@hes-so.ch>");
MODULE_DESCRIPTION ("Module skeleton");
MODULE_LICENSE ("GPL");
