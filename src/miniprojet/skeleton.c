// skeleton.c
#include <linux/init.h>		// needed for macros
#include <linux/fs.h>       // needed for device drivers
#include <linux/kernel.h>	// needed for debugging
#include <linux/module.h>	// needed by all modules
#include <linux/timer.h>	// need for timer
#include <linux/thermal.h>	// need for get cpu temp 
#include <linux/gpio.h>		// need for access GPIIO
#include <linux/moduleparam.h>	// needed for module parameters
#include <linux/device.h>  // needed for sysfs

#define LED 10
char * Led_labe = "LED";

static struct class* sysfs_class;
static struct device* sysfs_device;

enum Mode {AUTO, MAN};
static int current_mode = MAN;
static int  frequency_Hz = 2;		// 2, 5, 10, 20

const char *cpu_thermal = "cpu-thermal";
static struct timer_list my_timer;
static int led_state = 0;

// ------- Sysfs class  -------
ssize_t sysfs_show_mode(struct device* dev, struct device_attribute* attr, char* buf) {
    sprintf(buf, "%d\n", current_mode);  // copy formatted attribute to buf
    return strlen(buf);         // return buf size
}

ssize_t sysfs_store_mode(struct device* dev, struct device_attribute* attr, const char* buf, size_t count) {    
    current_mode = simple_strtol( // set val with buf (simple_strtol => convert string to int)
        buf,    // string buffer
        0,      // end char
        10);    // base (int)
    return count;                       // return value size
}

ssize_t sysfs_show_frequency(struct device* dev, struct device_attribute* attr, char* buf) {
    sprintf(buf, "%d\n", frequency_Hz);  // copy formatted attribute to buf
    return strlen(buf);         // return buf size
}

ssize_t sysfs_store_frequency(struct device* dev, struct device_attribute* attr, const char* buf, size_t count) {    
    frequency_Hz = simple_strtol( // set val with buf (simple_strtol => convert string to int)
        buf,    // string buffer
        0,      // end char
        10);    // base (int)
    return count;                   // return value size
}

DEVICE_ATTR(mode, 0664, sysfs_show_mode, sysfs_store_mode);

DEVICE_ATTR(frequency, 0664, sysfs_show_frequency, sysfs_store_frequency);

// ------- Timer Callback -------
static void timer_callback(struct timer_list *timer){
	// ---- method called all 1/2*f seconds----------

	int ret, f, temp, interval_ms;
	// reverse status led state
	led_state = !led_state;
	gpio_set_value(LED, led_state);

	// get and print cpu temp
	struct thermal_zone_device* thermal_zone = thermal_zone_get_zone_by_name (cpu_thermal);
	thermal_zone_get_temp(thermal_zone, &temp);

	// set frequency AUTO or MAN mode
	if(current_mode == AUTO){
		if(temp < 35000){
			f = 2;
		}
		else if(temp < 40000){
			f = 5;
		}
		else if (temp < 45000)
		{
			f = 10;
		}
		else{
			f = 20;
		}
	}else{
		f = frequency_Hz;
	}
	
	// process next interval depending on set frequency
	interval_ms = 1000/(2*f);
	
	// set next timer interval and restart
	ret = mod_timer(&my_timer, jiffies + msecs_to_jiffies(interval_ms));
	if (ret)
		pr_err("%s: Timer firing failed\n", __func__);
}

static int __init skeleton_init(void)
{
	int ret;
	pr_info ("Linux module fan management loaded\n");

	// ------ init Timer ------

	// set time
	timer_setup(&my_timer, timer_callback, CLOCK_MONOTONIC);
	pr_info("%s: Setup timer to fire in 2s (%ld)\n", __func__, jiffies);

	// set timer interval and start
	ret = mod_timer(&my_timer, jiffies + msecs_to_jiffies(2000));
	if (ret)
		pr_err("%s: Timer firing failed\n", __func__);
	printk("Timer Started\r\n");

	// ------ init GPIO ------

	int status;
    status = gpio_request(LED,Led_labe);

	gpio_direction_output(LED, led_state);

	// ------ init sys class ------
	status = 0;

    // init. class
    sysfs_class  = class_create(THIS_MODULE, "fan_management_class");

    // init. device
    sysfs_device = device_create(
        sysfs_class,        // class
        NULL,               // device parent
        0,                  // dev_t devt (définition du numéro de pilote)
        NULL,               // format (const char *)
        "fan_management"   // device's name
    );

    // installation des méthodes d'accès
    if (status == 0) status = device_create_file(
        sysfs_device,       // device
        &dev_attr_mode       // device_attribute struct address
    );

    if (status == 0) status = device_create_file(
        sysfs_device,       // device
        &dev_attr_frequency    // device_attribute struct address
    );

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

	// free GPIO (LED Status)
	gpio_free(LED);

	// remove access methods
    device_remove_file(sysfs_device, &dev_attr_mode);
    device_remove_file(sysfs_device, &dev_attr_frequency);

    // destroy device
    device_destroy(sysfs_class, 0);

    // destroy class
    class_destroy(sysfs_class);
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Glenn Muller <glenn.mullerar@hes-so.ch>");
MODULE_DESCRIPTION ("Linux module fan management");
MODULE_LICENSE ("GPL");
