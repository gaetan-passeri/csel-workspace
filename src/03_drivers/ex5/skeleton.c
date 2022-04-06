/* skeleton.c */
#include <linux/fs.h>          /* needed for device drivers */
#include <linux/init.h>        /* needed for macros */
#include <linux/kernel.h>      /* needed for debugging */
#include <linux/module.h>      /* needed by all modules */
#include <linux/uaccess.h>     /* needed to copy data to/from user */
#include <linux/device.h>      /* needed for sysfs handling */

// ---- data definition -------------------------------------------------------
struct skeleton_config {
    int id;
    long ref;
    char name[30];
    char descr[30];
};

static struct skeleton_config config;
static int val;
// ----------------------------------------------------------------------------

// ---- data access -----------------------------------------------------------
ssize_t sysfs_show_val(struct device* dev, struct device_attribute* attr, char* buf) {
    sprintf(buf, "%d\n", val);  // copy formatted attribute to buf
    return strlen(buf);         // return buf size
}

ssize_t sysfs_store_val(struct device* dev, struct device_attribute* attr, const char* buf, size_t count) {    
    val = simple_strtol( // set val with buf (simple_strtol => convert string to int)
        buf,    // string buffer
        0,      // end char
        10);    // base (int)
    return count;                       // return value size
}

DEVICE_ATTR(val, 0664, sysfs_show_val, sysfs_store_val);

ssize_t sysfs_show_cfg(struct device* dev, struct device_attribute* attr, char* buf){
    sprintf(buf, "%d %ld %s %s\n",
        config.id,
        config.ref,
        config.name,
        config.descr
    );

    return strlen(buf);
}

ssize_t sysfs_store_cfg(struct device* dev, struct device_attribute* attr, const char* buf, size_t count){
    sscanf(
        buf,            // src string
        "%d %ld %s %s",  // string data format
        &config.id,     // pointers to data
        &config.ref,    // |
        config.name,    // |
        config.descr    // |
    );

    return count;
}

DEVICE_ATTR(config, 0664, sysfs_show_cfg, sysfs_store_cfg);

// ----------------------------------------------------------------------------

// class and device structures
static struct class* sysfs_class;
static struct device* sysfs_device;

static int __init skeleton_init(void) {
    int status = 0;

    // init. class
    sysfs_class  = class_create(THIS_MODULE, "my_sysfs_class");

    // init. device
    sysfs_device = device_create(
        sysfs_class,        // class
        NULL,               // device parent
        0,                  // dev_t devt (définition du numéro de pilote)
        NULL,               // format (const char *)
        "my_sysfs_device"   // device's name
    );

    // installation des méthodes d'accès
    if (status == 0) status = device_create_file(
        sysfs_device,       // device
        &dev_attr_val       // device_attribute struct address
    );

    if (status == 0) status = device_create_file(
        sysfs_device,       // device
        &dev_attr_config    // device_attribute struct address
    );

    pr_info("Linux module skeleton loaded\n");

    return 0;
}

static void __exit skeleton_exit(void) {

    // remove access methods
    device_remove_file(sysfs_device, &dev_attr_val);
    device_remove_file(sysfs_device, &dev_attr_config);

    // destroy device
    device_destroy(sysfs_class, 0);

    // destroy class
    class_destroy(sysfs_class);

    pr_info("Linux module skeleton unloaded\n");
}


module_init(skeleton_init);
module_exit(skeleton_exit);

MODULE_AUTHOR("Gaetan Passeri <gaetan.passeri@master.hes-so.ch>");
MODULE_DESCRIPTION("Module skeleton");
MODULE_LICENSE("GPL");