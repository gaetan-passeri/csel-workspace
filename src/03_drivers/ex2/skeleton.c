/* skeleton.c */
#include <linux/cdev.h>        /* needed for char device driver */
#include <linux/fs.h>          /* needed for device drivers */
#include <linux/init.h>        /* needed for macros */
#include <linux/kernel.h>      /* needed for debugging */
#include <linux/module.h>      /* needed by all modules */
#include <linux/moduleparam.h> /* needed for module parameters */
#include <linux/uaccess.h>     /* needed to copy data to/from user */

#define BUFFER_SZ 1000

static char s_buffer[BUFFER_SZ];

static dev_t skeleton_dev;
static struct cdev skeleton_cdev;



static int skeleton_open(struct inode* i, struct file* f) {
    return 0;
}

static int skeleton_release(struct inode* i, struct file* f) {
    return 0;
}

static ssize_t skeleton_read(struct file* f, char __user* buf, size_t count, loff_t* off) {
    // compute remaining bytes to copy, update count and pointers
    ssize_t remaining = BUFFER_SZ - (ssize_t)(*off);
    char* ptr         = s_buffer + *off;
    if (count > remaining) count = remaining;
    *off += count;

    // copy required number of bytes
    if (copy_to_user(buf, ptr, count) != 0) count = -EFAULT; // EFAULT = bad adress
    // NB : copy_to_user retourne le nombre d'octets qui n'ont pas pu êtres copiés!

    pr_info("skeleton: read operation... read=%ld\n", count);
    return count; // retourne le nombre d'octets écris dans l'espace utilisateur
}

static ssize_t skeleton_write(struct file* f, const char __user* buf, size_t count, loff_t* off) {

    ssize_t remaining = BUFFER_SZ - (ssize_t)(*off);
    pr_info("skeleton: at%ld\n", (unsigned long)(*off));

    // check if still remaining space to store additional bytes
    if (count >= remaining) count = -EIO;

    if(count > 0){
        char* ptr = s_buffer + *off;
        *off += count;    
        ptr[count] = 0; // make sure string is null terminated
        if(copy_from_user(ptr, buf, count)) count = -EFAULT;
    }

    pr_info("skeleton: write operation... written=%ld\n", count);

    return count; // retourne le nombre d'octets copiés
}

static struct file_operations skeleton_fops = {
    .owner   = THIS_MODULE,
    .open    = skeleton_open,
    .read    = skeleton_read,
    .write   = skeleton_write,
    .release = skeleton_release,
};

static int __init skeleton_init(void) {
    int status = alloc_chrdev_region(&skeleton_dev, 0, 1, "io_string_module");
    if (status == 0) {
        cdev_init(&skeleton_cdev, &skeleton_fops);
        skeleton_cdev.owner = THIS_MODULE;
        status              = cdev_add(&skeleton_cdev, skeleton_dev, 1);
    }

    pr_info("Linux module skeleton loaded\n");
    return 0;
}

static void __exit skeleton_exit(void) {
    cdev_del(&skeleton_cdev);
    unregister_chrdev_region(skeleton_dev, 1);
    pr_info("Linux module skeleton unloaded\n");
}


module_init(skeleton_init);
module_exit(skeleton_exit);

MODULE_AUTHOR("Gaetan Passeri <gaetan.passeri@master.hes-so.ch>");
MODULE_DESCRIPTION("Module skeleton");
MODULE_LICENSE("GPL");