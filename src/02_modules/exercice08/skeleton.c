// skeleton.c
#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging
#include <linux/gpio.h>      // need for gpio
#include <linux/interrupt.h>

#define K1 0
#define K2 2
#define K3 3

char* k1_label = "k1";
char* k2_label = "k2";
char* k3_label = "k3";

irqreturn_t gpio_callback(int irq, void *dev_id){
    
    pr_info("interrupt %s: rise",(char*) dev_id);
    
    return IRQ_HANDLED;
}

static int __init skeleton_init(void)
{
	pr_info ("Linux module 08 skeleton loaded\n");

    int status;
    status = gpio_request(K1,k1_label);
    if(status == 0){
        request_irq(gpio_to_irq(K1) ,gpio_callback, IRQF_TRIGGER_FALLING, k1_label, k1_label);
    }
    
    status = gpio_request(K2,k2_label);
    if(status == 0){
        request_irq(gpio_to_irq(K2) ,gpio_callback, IRQF_TRIGGER_FALLING, k2_label, k2_label);
    }

    status = gpio_request(K3,k3_label);
    if(status == 0){
        request_irq(gpio_to_irq(K3) ,gpio_callback, IRQF_TRIGGER_FALLING, k3_label, k3_label);
    }
	return 0;
}

static void __exit skeleton_exit(void)
{
    gpio_free(K1);
    free_irq(gpio_to_irq(K1), k1_label);

    gpio_free(K2);
    free_irq(gpio_to_irq(K2), k2_label);

    gpio_free(K3);
    free_irq(gpio_to_irq(K3), k3_label);
	pr_info ("Linux module 08 unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Glenn Muller <glenn.mullerar@hes-so.ch>");
MODULE_DESCRIPTION ("Module Exercice08");
MODULE_LICENSE ("GPL");
