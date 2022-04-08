#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging

#include <linux/moduleparam.h>	// needed for module parameters

#include <linux/ioport.h>
#include <linux/io.h>


#define CHIP_ID_ADD 	0x01c14200
#define TEMP_CPU_ADD 	0x01C25000
#define MACC_ADD		0x01C30000

static struct resource* res[3];


static int __init skeleton_init(void)
{

	unsigned int chipid[4];
	unsigned int mac_addr[2];

	unsigned char *reg_chipid;
	unsigned char *reg_temp_sensor;
	unsigned char *reg_mac;

	long temp;

	pr_info ("Linux module 05 Input Output loaded\n");

	res[0] = request_mem_region(CHIP_ID_ADD, 0x1000, "allwiner sid");
	
	res[1] = request_mem_region(TEMP_CPU_ADD, 0x1000, "allwiner h5 ths");
	
	res[2] = request_mem_region(MACC_ADD, 0x1000, "allwiner h5 emac");

	if(res[1] == 0){
		pr_info("CPU THP : ERROR request mem region");
	}
		
	reg_chipid = ioremap(CHIP_ID_ADD, 0x1000);
	reg_temp_sensor = ioremap(TEMP_CPU_ADD, 0x1000);
	reg_mac = ioremap(MACC_ADD, 0x1000);

	chipid[0] = ioread32(reg_chipid+0x200);
	chipid[1] = ioread32(reg_chipid+0x204);
	chipid[2] = ioread32(reg_chipid+0x208);
	chipid[3] = ioread32(reg_chipid+0x20c);

	pr_info("chipid=%08x'%08x'%08x'%08x\n",
	 	chipid[0], chipid[1], chipid[2], chipid[3]);
	
	temp = -1191 * (int)ioread32(reg_temp_sensor+0x80) / 10 + 223000;
	pr_info("CPU's temperature is %ld",temp);

	mac_addr[0] = ioread32(reg_mac+0x50);
	mac_addr[1] = ioread32(reg_mac+0x54);
	pr_info("mac-addr=%02x:%02x:%02x:%02x:%02x:%02x\n",
			(mac_addr[1]>> 0) & 0xff,
			(mac_addr[1]>> 8) & 0xff,
			(mac_addr[1]>>16) & 0xff,
			(mac_addr[1]>>24) & 0xff,
			(mac_addr[0]>> 0) & 0xff,
			(mac_addr[0]>> 8) & 0xff);

	iounmap(reg_chipid);
	iounmap(reg_temp_sensor);
	iounmap(reg_mac);


	return 0;
}

static void __exit skeleton_exit(void)
{
	release_mem_region(CHIP_ID_ADD, 0x1000);
	release_mem_region(TEMP_CPU_ADD, 0x1000);
	release_mem_region(MACC_ADD, 0x1000);
	pr_info ("Linux module Input Output unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Glenn Muller <glenn.mullerar@hes-so.ch>");
MODULE_DESCRIPTION ("Module Input and Output");
MODULE_LICENSE ("GPL");