//#include <common.h>
//#include <asm/io.h>
//#include <asm/arch/hardware.h>
//#include <asm/arch/gpio.h>
//#include <asm/arch/clk.h>

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/delay.h>

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sysctl.h>
#include <linux/gpio_keys.h>

#include <asm/uaccess.h>
#include <mach/at91_twi.h>
#include <mach/at91_pmc.h>
#include <mach/at91_pio.h>
#include <asm/gpio.h>
#include <linux/clk.h>
#include <asm/gpio.h>

#include "SetLCD.h"

//----------------------------------------------------------------------------
/* # mknod /dev/setlcd c 152 0 */
// Device information
#define SetLCD_MAJOR 152

// Address
#define MAPBASE 0xFFFFC000
#define MAPSIZE 16384

static int debug_enable = 0;
module_param(debug_enable, int, 0);
MODULE_PARM_DESC(debug_enable, "Enable module debug mode.");

static void *base;
struct file_operations setlcd_fops;


#define bright          0x0000 //0x0680

// 0:darkest, 2: brignterest
const unsigned int BRIGHTNESS_TABLE[16] = {
	//0x0000, 0x0680, 0x0B80
	0x0000, 0x0000/*0x0180*/, 0x0000/*0x0280*/, 0x0180, 0x0280, 0x0380, 
	0x0480, 0x0580, 0x0680, 0x0780, 0x0880, 0x0980,
	0x0a80, 0x0b80, 0x0c80, 0x0d80
};

// 0:Lowest contrast, 15:Higheset contrast
const unsigned int CONTRAST_TABLE[16] = {
	0x0802, 0x0802, 0x0802, 0x0802, 0x0812, 0x0822, 
	0x0832, 0x0842, 0x0852, 0x0862, 0x0872, 0x0882, 
	0x0892, 0x08a2, 0x08b2, 0x08c2

/*	0x0802, 0x0812, 0x0822, 0x0832, 0x0842, 0x0852, 
	0x0862, 0x0872, 0x0882, 0x0892, 0x08a2, 0x08b2, 
	0x08c2, 0x08d2, 0x08e2, 0x08f2
*/
};

// 0:Standby, 1:Operation
const unsigned int STANBY_TABLE[2] = {
	0x0006, 0x0106
};

//-----------------------------------------------------------------------------------
static void LCD_SOUT(unsigned int data)
{
        unsigned int i;

       // DI  = L set
        at91_set_gpio_output(AT91_PIN_PC29, 0);
        // CS  = H set
        at91_set_gpio_output(AT91_PIN_PC3, 1);
        // SCK = H set
        at91_set_gpio_output(AT91_PIN_PC28, 1);

        // Send data to serial bit by bit
        for(i=0;i<12;i++) {

                // SCK = L set
                at91_set_gpio_output(AT91_PIN_PC28, 0);
                // CS  = L set
                at91_set_gpio_output(AT91_PIN_PC3, 0);

                // data 1 or 0
                if( (data & 1) == 1) {
                        // DI  = H set
                        at91_set_gpio_output(AT91_PIN_PC29, 1);
                } else {
                        // DI  = L set
                        at91_set_gpio_output(AT91_PIN_PC29, 0);
                }
                data >>= 1;
                // SCK = H set
                at91_set_gpio_output(AT91_PIN_PC28, 1);
        }
        // CS  = H set
        at91_set_gpio_output(AT91_PIN_PC3, 1);
}

void urp2_bcp_sleep(unsigned char mode)
{
        at91_set_gpio_output(AT91_PIN_PE0, mode);
        at91_set_gpio_output(AT91_PIN_PE1, mode);
        at91_set_gpio_output(AT91_PIN_PE2, mode);
        at91_set_gpio_output(AT91_PIN_PE3, mode);
}


void urp2_ddu_sleep(unsigned char mode)
{
        at91_set_gpio_output(AT91_PIN_PE6, mode);
}



//-----------------------------------------------------------------------------------
static int setlcd_open(struct inode *inode, struct file *file)
{
	return 0;
}


static int setlcd_release(struct inode *inode, struct file *file)
{
	return 0;
}


static ssize_t setlcd_read(struct file *file, char *buf, size_t count,
                loff_t *ptr)
{
	return 0;
}


static ssize_t setlcd_write(struct file *file, const char *buf,
                size_t count, loff_t * ppos)
{
	return 0;
}

static int setlcd_ioctl(struct inode *inode, struct file *file,
                unsigned int cmd, unsigned long arg)
{	
	int ret=0;

	switch (cmd)
	{
		case CMD_SET_BRIGHTNESS:
		{
			int val=(unsigned int)arg;
			//if (val>2) {
			if (val>15) {
				printk("Please set brightness 0-2\n");
				return ret;
			}
			
			//printk("brightness set value = 0x%x\n", BRIGHTNESS_TABLE[val]);
			LCD_SOUT( BRIGHTNESS_TABLE[val]);
			if(val<=1){
		                at91_sys_write(AT91_PIOE + PIO_PDR,0x00000800);
		                at91_sys_write(AT91_PIOE + PIO_BSR,0x00000800);
				at91_sys_write(AT91_PMC_PCKR(3),0x00000010);
				at91_sys_write(AT91_PMC_SCER,0x00000800);
				LCD_SOUT( BRIGHTNESS_TABLE[0]);
			}
			else{
				at91_sys_write(AT91_PMC_SCDR,0x00000800);
		                at91_sys_write(AT91_PIOE + PIO_PER,0x00000800);
                        	at91_set_gpio_output(AT91_PIN_PE11, 1);
				LCD_SOUT( BRIGHTNESS_TABLE[val]);
			}
		}
		break;

		case CMD_SET_CONTRAST:
		{
			int val=(unsigned int)arg;
			if (val>15) {
				printk("Please set contrast 0-15\n");
				return ret;
			}
			
			//printk("contrast set value = 0x%x\n", CONTRAST_TABLE[val]);
			LCD_SOUT(CONTRAST_TABLE[val]);
		}
			break;

		case CMD_SET_ONOFF:
		{
			int val=(unsigned int)arg;
			if (val>DDU_LCD_ON){
				printk("Please enter 0-3\n");
				return ret;
			}

			if (val==BCP_LCD_OFF){
				at91_set_gpio_output(AT91_PIN_PE4, 0);
				LCD_SOUT(STANBY_TABLE[0]);
				//urp2_bcp_sleep(0);
			} else if (val==BCP_LCD_ON){
				LCD_SOUT(STANBY_TABLE[1]);
				at91_set_gpio_output(AT91_PIN_PE4, 1);
				//urp2_bcp_sleep(1);
			} else if (val==DDU_LCD_OFF){
				at91_set_gpio_output(AT91_PIN_PE11, 0);
				LCD_SOUT(STANBY_TABLE[0]);
				//urp2_ddu_sleep(0);
			}else if (val==DDU_LCD_ON){
				LCD_SOUT(STANBY_TABLE[1]);
				at91_set_gpio_output(AT91_PIN_PE11, 1);
				//urp2_ddu_sleep(1);
			}
		}
		break;

		case CMD_SET_DATA:
			LCD_SOUT((unsigned int)arg);
			break;

		default:
			ret = -1;
			break;
	}
	return ret;
}

//------------------------------------------------------------------------------------------------------

static int setlcd_start(void)
{
	
	//LCD_SOUT(bright);

	return 0;
}

static int setlcd_end(void)
{
	//LCD_SOUT(0x1000);

	return 0;
}

static int __init addr_init(void)
{
	base = ioremap(MAPBASE, MAPSIZE);
	if (base == NULL) {
		printk(KERN_INFO "Cannot get base address, ioremap error\n");
		return -ENOMEM;
	}
	return 0;
}

static void __init setlcd_init_error(void) 
{
	iounmap(base);
	unregister_chrdev(SetLCD_MAJOR, "setlcd");
	printk("init error!\n");
}


static int __init setlcd_init(void)
{
	int ret;

	printk("setlcd_init start..\n");
	ret = register_chrdev(SetLCD_MAJOR, "setlcd", &setlcd_fops);
	if (ret < 0){
		setlcd_init_error();
		return ret;
	}

	ret = addr_init();
	if (ret < 0){
		setlcd_init_error();
		return ret;
	}

	ret = setlcd_start();
	if (ret < 0){
		setlcd_init_error();
		return ret;
	}
	

	printk("setlcd_init OK!\n");
	return 0;
}

static void __exit setlcd_exit(void)
{
	printk("setlcd_exit\n");
	setlcd_end();
	unregister_chrdev(SetLCD_MAJOR, "setlcd");
}


struct file_operations setlcd_fops = {
	owner:   THIS_MODULE,
	read:    setlcd_read,
	write:   setlcd_write,
	ioctl:   setlcd_ioctl,
	open:    setlcd_open,
	release: setlcd_release,
};

module_init(setlcd_init);
module_exit(setlcd_exit);

MODULE_AUTHOR (" ");
MODULE_DESCRIPTION("SetLCD Driver");
MODULE_LICENSE("GPL v2");


