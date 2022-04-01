// skeleton.c
#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging

#include <linux/moduleparam.h>	// needed for module parameters
#include <linux/gfp.h>
#include <linux/slab.h>



static char* text = "dummy text";
module_param(text, charp, 0664);
static int  elements = 1;
module_param(elements, int, 0);

struct element
{
	char *str;
	int id;
	struct list_head node;
};

static LIST_HEAD (my_list);

static int __init skeleton_init(void)
{
	pr_info ("Linux module 01 skeleton loaded\n");
	pr_debug ("  text: %s\n  elements: %d\n", text, elements);
	int i = 0;
	for(i;i < elements;i++){
		struct element* ele;
		ele = kmalloc(sizeof(*ele), GFP_KERNEL); // create a new element if (ele != NULL)
		ele->id = i;
		ele->str = text;
		list_add_tail(&ele->node, &my_list); // add element at the end of the list }
		pr_debug(" element with id %d and str %s has been added to the list",ele->id,ele->str);
	}

	return 0;
}

static void __exit skeleton_exit(void)
{
	struct element* ele;
	// list_for_each_entry(ele, &my_list, node) {// iterate over the whole list
	// 	list_del(&ele->node);
	// 	kfree(ele);
	// 	pr_debug("element free");
	// }
	while(!list_empty(&my_list)){
		ele = list_entry(my_list.next, struct element, node);
		list_del(&ele->node);
		kfree(ele);
		pr_debug("element free");
	} 
	pr_info ("Linux module skeleton unloaded\n");

}


module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION ("Module skeleton");
MODULE_LICENSE ("GPL");
