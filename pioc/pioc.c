/*
 *  pioc.c
 *
 *  Revision history:
 *  16 May 2008: 
 */

// Module header
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/delay.h>

// Board specific header
//#include <asm/arch/gpio.h>
//#include <asm/arch/at91sam9261.h>
//#include <asm/arch/at91sam9261_matrix.h>
//#include <asm/arch/at91sam926x_mc.h>
//#include <asm/io.h>
#include <mach/gpio.h>
#include <mach/at91sam9263.h>
#include <mach/at91sam9263_matrix.h>
#include <mach/at91sam9_sdramc.h>
#include <asm/io.h>

#include "atmel_serial.h"
#include "pio.h"

// Device information
#define PIOC_MAJOR 149

// Address
#define MAPBASE 0xFFFFC000
#define MAPSIZE 16384

static int debug_enable = 0;
module_param(debug_enable, int, 0);
MODULE_PARM_DESC(debug_enable, "Enable module debug mode.");

static void *base;

struct file_operations pioc_fops;

static int pioc_open(struct inode *inode, struct file *file)
{
	return 0;
}


static int pioc_release(struct inode *inode, struct file *file)
{
	return 0;
}


static ssize_t pioc_read(struct file *file, char *buf, size_t count,
                loff_t *ptr)
{
	return 0;
}


static ssize_t pioc_write(struct file *file, const char *buf,
                size_t count, loff_t * ppos)
{
	return 0;
}

static int pioc_ioctl(struct inode *inode, struct file *file,
                unsigned int cmd, unsigned long arg)
{
	PIOCstr *piostr = (PIOCstr *)arg;
	int type = piostr->type;
	int pinID = piostr->pinID;
	int retval=0;
	switch (cmd) {
		case CMD_PIOC_GET:
			switch (type) {
				case 0:
					retval = at91_get_gpio_value(PIN_BASE + pinID);
					break;
				case 1:
					retval = at91_get_gpio_value(PIN_BASE + 0x20 + pinID);
					break;
				case 2:
					retval = at91_get_gpio_value(PIN_BASE + 0x40 + pinID);
					break;
				case 3:
					retval = at91_get_gpio_value(PIN_BASE + 0x60 + pinID);
					break;
				case 4:
					retval = at91_get_gpio_value(PIN_BASE + 0x80 + pinID);
					break;
				default:
					break;
			}
			break;
		case CMD_PIOC_SET:
			switch (type) {
				case 0:
					at91_set_gpio_output(PIN_BASE + pinID, 1);
					break;
				case 1:
					at91_set_gpio_output(PIN_BASE + 0x20 + pinID, 1);
					break;
				case 2:
					at91_set_gpio_output(PIN_BASE + 0x40 + pinID, 1);
					break;
				case 3:
					at91_set_gpio_output(PIN_BASE + 0x60 + pinID, 1);
					break;
				case 4:
					at91_set_gpio_output(PIN_BASE + 0x80 + pinID, 1);
					break;
				default:
					break;
			}
			break;
		case CMD_PIOC_CLEAR:
			switch (type) {
				case 0:
					at91_set_gpio_output(PIN_BASE + pinID, 0);
					break;
				case 1:
					at91_set_gpio_output(PIN_BASE + 0x20 + pinID, 0);
					break;
				case 2:
					at91_set_gpio_output(PIN_BASE + 0x40 + pinID, 0);
					break;
				case 3:
					at91_set_gpio_output(PIN_BASE + 0x60 + pinID, 0);
					break;
				case 4:
					at91_set_gpio_output(PIN_BASE + 0x80 + pinID, 0);
					break;
				default:
					break;
			}
			break;
		case CMD_PIOC_INPUT:
			switch (type) {
				case 0:
					at91_set_gpio_input(PIN_BASE + pinID, 0);
					break;
				case 1:
					at91_set_gpio_input(PIN_BASE + 0x20 + pinID, 0);
					break;
				case 2:
					at91_set_gpio_input(PIN_BASE + 0x40 + pinID, 0);
					break;
				case 3:
					at91_set_gpio_input(PIN_BASE + 0x60 + pinID, 0);
					break;
				case 4:
					at91_set_gpio_input(PIN_BASE + 0x80 + pinID, 0);
					break;
				default:
					break;
			}
			break;

		default:
			break;
	}
	return retval;
}

static int __init addr_init(void)
{
	// Retrieve virtual address of USART0 for controlling it's registers
	base = ioremap(MAPBASE, MAPSIZE);
	if (base == NULL) {
		printk(KERN_INFO "Cannot get base address, ioremap error\n");
		return -ENOMEM;
	}
	if (debug_enable) {
		printk(KERN_INFO "Bind 0x%x to VA 0x%x\n", MAPBASE, (unsigned int)base);
	}
	return 0;
}

static void __init pioc_init_error(void) {
	printk(KERN_INFO "Error registering pioc device\n");
	iounmap(base);
	unregister_chrdev(PIOC_MAJOR, "pioc");
}

static int __init pioc_init(void)
{
	int ret;
	printk(KERN_INFO "PIOC module init - debug mode is %s\n",
            debug_enable ? "enabled" : "disabled");

	ret = register_chrdev(PIOC_MAJOR, "pioc", &pioc_fops);
	if (ret < 0) {
		pioc_init_error();
		return ret;
	}
	if (debug_enable) {
		printk(KERN_INFO "Sam: registered module successfully!\n");
	}

	/* Init processing here... */
	ret = addr_init();
	if (ret < 0) {
		pioc_init_error();
		return ret;
	}
	return 0;
}


static void __exit pioc_exit(void)
{
	printk(KERN_INFO "PIOC module exits\n");
	unregister_chrdev(PIOC_MAJOR, "pioc");
}


struct file_operations pioc_fops = {
	owner:   THIS_MODULE,
	read:    pioc_read,
	write:   pioc_write,
	ioctl:   pioc_ioctl,
	open:    pioc_open,
	release: pioc_release,
};

module_init(pioc_init);
module_exit(pioc_exit);

MODULE_AUTHOR(" ");
MODULE_DESCRIPTION("PIO access module");
MODULE_LICENSE("GPL v2");
