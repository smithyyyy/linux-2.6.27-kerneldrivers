/*
 *
 *  URP2 Hardware Peripherals Control.
 *
 *  Revision history:
 *  16 May 2008: initial version
 */

// Module header
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <mach/gpio.h>
#include <mach/at91sam9263.h>
#include <mach/at91sam9263_matrix.h>
#include <mach/at91sam9_sdramc.h>
#include <asm/io.h>

#include "atmel_serial.h"
#include "urp2.h"

// Device information
#define URP2_MAJOR 42

// Address
#define MAPBASE AT91SAM9263_BASE_US1
#define MAPSIZE 16384

//battery level
#define IN1_ADDRESS	0x00
#define IN2_ADDRESS	0x08
#define	ADC_LSB		3220/(32*256)	/* VA = 3.22 V */
#define INT1_FACTOR	212/100 

// Marcos for writing ATMEL registers
#define UART_PUT_CR(port,v)     __raw_writel(v, port + ATMEL_US_CR)
#define UART_GET_MR(port)       __raw_readl(port + ATMEL_US_MR)
#define UART_PUT_MR(port,v)     __raw_writel(v, port + ATMEL_US_MR)
#define UART_PUT_IER(port,v)    __raw_writel(v, port + ATMEL_US_IER)
#define UART_PUT_IDR(port,v)    __raw_writel(v, port + ATMEL_US_IDR)
#define UART_GET_IMR(port)      __raw_readl(port + ATMEL_US_IMR)
#define UART_GET_CSR(port)      __raw_readl(port + ATMEL_US_CSR)
#define UART_GET_CHAR(port)     __raw_readl(port + ATMEL_US_RHR)
#define UART_PUT_CHAR(port,v)   __raw_writel(v, port + ATMEL_US_THR)
#define UART_GET_BRGR(port)     __raw_readl(port + ATMEL_US_BRGR)
#define UART_PUT_BRGR(port,v)   __raw_writel(v, port + ATMEL_US_BRGR)
#define UART_PUT_RTOR(port,v)   __raw_writel(v, port + ATMEL_US_RTOR)
#define UART_PUT_PTCR(port,v)   __raw_writel(v, port + ATMEL_PDC_PTCR)
#define UART_GET_PTSR(port)     __raw_readl(port + ATMEL_PDC_PTSR)
#define UART_PUT_RPR(port,v)    __raw_writel(v, port + ATMEL_PDC_RPR)
#define UART_GET_RPR(port)      __raw_readl(port + ATMEL_PDC_RPR)
#define UART_PUT_RCR(port,v)    __raw_writel(v, port + ATMEL_PDC_RCR)
#define UART_PUT_RNPR(port,v)   __raw_writel(v, port + ATMEL_PDC_RNPR)
#define UART_PUT_RNCR(port,v)   __raw_writel(v, port + ATMEL_PDC_RNCR)
#define UART_PUT_TPR(port,v)    __raw_writel(v, port + ATMEL_PDC_TPR)
#define UART_PUT_TCR(port,v)    __raw_writel(v, port + ATMEL_PDC_TCR)
#define UART_GET_FIDI(port) __raw_readl(port + ATMEL_US_FIDI)
#define UART_GET_TTGR(port) __raw_readl(port + ATMEL_US_TTGR)
#define UART_PUT_FIDI(port,v)   __raw_writel(v, port + ATMEL_US_FIDI)
#define UART_PUT_TTGR(port,v)   __raw_writel(v, port + ATMEL_US_TTGR)

#define UART_GET_MAN(port)       __raw_readl(port + ATMEL_US_MAN)
#define UART_PUT_MAN(port,v)     __raw_writel(v, port + ATMEL_US_MAN)

static int DEBUG = 0;
module_param(DEBUG, int, 0);
MODULE_PARM_DESC(DEBUG, "Enable module debug mode.");

static int WITHOUT_SMT = 0;
module_param(WITHOUT_SMT, int, 0);
MODULE_PARM_DESC(WITHOUT_SMT, "Disable SMT control.");

static void *base;

//Merged OLD-MAIN BT-2110: tempory storage for user data
static unsigned char g_data[URP_DATA_NUM][URP_DATA_MAX];

struct file_operations urp2_fops;

void urp2_rf_start(void);
void urp2_rf_stop(void);

void urp2_485_direct(void);
void urp2_485_reverse(void);

void urp2_rf_start(void) {
	if (!WITHOUT_SMT) {
		// Set PIN PB14
		//at91_set_gpio_output(PIN_BASE + 0x20 + 14, 1);
		// Set PIN PD5
		// Set PIN PD6
		at91_set_gpio_output(PIN_BASE + 0x60 + 5, 1);
		at91_set_gpio_output(PIN_BASE + 0x60 + 6, 1);
		if (DEBUG) 
			printk(KERN_INFO "URP RF ON\n");
	}
}

void urp2_rf_stop(void) {
	if (!WITHOUT_SMT) {
		// Clear PIN PB14
		//at91_set_gpio_output(PIN_BASE + 0x20 + 14, 0);
		// Clear PIN PD5
		// Clear PIN PD6
		at91_set_gpio_output(PIN_BASE + 0x60 + 5, 0);
		at91_set_gpio_output(PIN_BASE + 0x60 + 6, 0);
		if (DEBUG)
			printk(KERN_INFO "URP RF OFF\n");
	}
}

void urp2_485_direct(void) {
	unsigned int mode;

	if (base == NULL) {
		return;
	}

	/*
	 * USART Mode Register
	 * ONE BIT = 1
	 * MODSYNC start bit is a 1 to 0 transition
	 * MAN Manchester Encoder/Decoder Enable
	 * PAR: No parity
	 * SYNC: operates at Asynchronous Mode
	 * CHRL: 8 bits
	 * USCLKS: MCK
	 * USART_MODE: RS485
	 */
	mode = 0xe00008c1;
	UART_PUT_MR(base, mode);

	/*
	 * USART Manchester Configuration Register
	 * DRIFT = 1
	 * RX_MPOL = 0
	 * TX_MPOL = logic one => 0-to-1 transition
	 */
	 mode = 0x30011004;
	 UART_PUT_MAN(base, mode);

}

void urp2_485_reverse(void) {
	unsigned int mode;

	if (base == NULL) {
		return;
	}

	/*
	 * USART Mode Register
	 * ONE BIT = 1
	 * MODSYNC start bit is a 1 to 0 transition
	 * MAN Manchester Encoder/Decoder Enable
	 * PAR: No parity
	 * SYNC: operates at Asynchronous Mode
	 * CHRL: 8 bits
	 * USCLKS: MCK
	 * USART_MODE: RS485
	 */
	mode = 0xe00008c1;
	UART_PUT_MR(base, mode);

	/*
	 * USART Manchester Configuration Register
	 * DRIFT = 1
	 * RX_MPOL = 0
	 * TX_MPOL = logic one => 1-to-0 transition
	 */
	 mode = 0x30010004;
	 UART_PUT_MAN(base, mode);
}

void urp2_bcp_led(int mode)
{
	/*unsigned char onoff[4], i;

	for (i=0;i<4;i++)
		onoff[i] = mode>>i & 0x01;
	
        at91_set_gpio_output(AT91_PIN_PE0, onoff[1]);
        at91_set_gpio_output(AT91_PIN_PE1, onoff[0]);
        at91_set_gpio_output(AT91_PIN_PE2, onoff[3]);
        at91_set_gpio_output(AT91_PIN_PE3, onoff[2]);*/

	unsigned char led, onoff;

	led = mode>>8;
	onoff = mode & 0xFF;
	
	printk("mode=%x, led=%x, onoff=%x\n", mode, led, onoff);
	if (led==URP_BCP_LEDALL) {
		at91_set_gpio_output(AT91_PIN_PE0, onoff);
        at91_set_gpio_output(AT91_PIN_PE1, onoff);
        at91_set_gpio_output(AT91_PIN_PE2, onoff);
        at91_set_gpio_output(AT91_PIN_PE3, onoff);
	}
	else {
		if (led==URP_BCP_LED1)
			at91_set_gpio_output(AT91_PIN_PE1, onoff);
		else if (led==URP_BCP_LED2)
			at91_set_gpio_output(AT91_PIN_PE0, onoff);
		else if (led==URP_BCP_LED3)
			at91_set_gpio_output(AT91_PIN_PE3, onoff);
		else if (led==URP_BCP_LED4)
			at91_set_gpio_output(AT91_PIN_PE2, onoff);
	}
}


void urp2_ddu_led(unsigned char mode)
{
        at91_set_gpio_output(AT91_PIN_PE6, mode);
}

// 20100513 WFSZE add to check whether it is DDU or BCP 
int urp2_isDDU(void)
{
	unsigned char
		inbit= 0;
	inbit = at91_get_gpio_value(AT91_PIN_PE22);
	return (0== inbit)?255:0;
}

// Mantis 6239: DDU SN number not in sequence, 
// for detecting power loss
static int urp2_is_powersupply_healthy(void)
{
	return at91_get_gpio_value(AT91_PIN_PD4);
}

static unsigned short read_battery_level(unsigned char address)
{
	int i;
	unsigned char
		dout =0,
		inbit=0;
	unsigned short
		din  =0;

	dout = address;
	din = 0;

	at91_set_gpio_output(AT91_PIN_PB15, 0);		/* SPI1_CS0 Low*/
	for(i=0;i<16;i++) {
		at91_set_gpio_output(AT91_PIN_PB14, 0); /*SPI1_SCLK Low*/

		// DOUT: 8 bit control data
		at91_set_gpio_output(AT91_PIN_PB13, (dout & 0x80)?1:0);

		// DIN: 8 bit data, start reading at the 5th clock cycle
		inbit = at91_get_gpio_value(AT91_PIN_PB12);
		din |= inbit;
	 
		at91_set_gpio_output(AT91_PIN_PB14, 1);	/* SPI1_SCLK High*/

		din <<= 1;
		dout <<= 1;
	 }
	 at91_set_gpio_output(AT91_PIN_PB15, 1);	/* SPI1_CS0 High*/
	
	 return din;
}

int urp2_RTC_battery_level(void) 
{
	unsigned short
		din= 0,
		loop=0;
//	int battery_level=0, rtcbattstatus = 0;
	int battery_level=0;

	loop= 0;
//	at91_set_gpio_output(AT91_PIN_PB11, 0);	
	at91_set_gpio_output(AT91_PIN_PB11, 1);	

#if 0
	while (loop++ < 100) rtcbattstatus = at91_get_gpio_value(AT91_PIN_PB6);

        if (rtcbattstatus) battery_level= 0;	// no battery exist
	else
	{		
		loop= 0;
		while (loop++ <10) 
			din = read_battery_level(IN2_ADDRESS);
		battery_level = (din * ADC_LSB);
	}
#endif

	loop= 0;
	while (loop++ <10) 
		din = read_battery_level(IN2_ADDRESS);
	battery_level = (din * ADC_LSB);

	at91_set_gpio_input(AT91_PIN_PB11, 0);
	return battery_level;
}

int urp2_SAM_battery_level(void) 
{
	unsigned short
		din =0,
		loop=0;

	int
		battery_level=0;

	at91_set_gpio_output(AT91_PIN_PB15, 1);    // SPI1_NPCS0
	at91_set_gpio_output(AT91_PIN_PB13, 0);    // SPI1_MOSI
	at91_set_gpio_output(AT91_PIN_PB14, 1);    // SPI1_SPCK

	// read backup battery supervise battery
//	at91_set_gpio_output(AT91_PIN_PB11, 0);
	at91_set_gpio_output(AT91_PIN_PB11, 1);
	udelay(10);
	while (loop++ <10) 
		din = read_battery_level(IN1_ADDRESS);
	
	battery_level = (din * ADC_LSB) * INT1_FACTOR;

	at91_set_gpio_input(AT91_PIN_PB11, 0);
	return battery_level;
}

static int urp2_open(struct inode *inode, struct file *file)
{
	return 0;
}


static int urp2_release(struct inode *inode, struct file *file)
{
	return 0;
}


static ssize_t urp2_read(struct file *file, char *buf, size_t count,
                loff_t *ptr)
{
	return 0;
}


static ssize_t urp2_write(struct file *file, const char *buf,
                size_t count, loff_t * ppos)
{
	return 0;
}

static int urp2_ioctl(struct inode *inode, struct file *file,
                unsigned int cmd, unsigned long arg)
{
	int retval=0;
	switch (cmd) {
		case URP_RESET:
			break;
		case URP_485:
			if (arg == URP_485_DIRECT) {
				urp2_485_direct();
			} else if (arg == URP_485_REVERSE) {
				urp2_485_reverse();
			}
			break;
		case URP_IO:
			break;
		case URP_232:
			break;
		case URP_FULLMODEM:
			break;
		case URP_IRDA:
			break;
		case URP_RF:
			if (arg == URP_DEVICE_ON) {
				urp2_rf_start();
			} else {
				urp2_rf_stop();
			}
		case URP_BCP_LED:
				urp2_bcp_led(arg);
			break;
		case URP_DDU_LED:
				urp2_ddu_led(arg);
			break;
		case URP_IMMOBILIZER:
			// Mantis 6648: Immobilizer Speed Up
			// Add URP_IMMOBILIZER_GET => to obtain PE 29 value
			// In order to let main application to decide UI output
			if (arg == URP_IMMOBILIZER_ON ||
				arg == URP_IMMOBILIZER_OFF) {
				at91_set_gpio_output(AT91_PIN_PE29, arg);
			} else {
				retval = at91_get_gpio_value(AT91_PIN_PE29);
			}
			break;
			// Mantis 6648: End

		//Merged OLD-MAIN BT-2110: read indexed storage
		case URP_STORAGE_READ:
		{
			urp_storage inData;
			unsigned long ret=0;

			if (copy_from_user((void *)&inData, (const void *)arg, sizeof(inData)))
				return -EFAULT;
			if (inData.index >= URP_DATA_MAX)
				return -EFAULT;
			memcpy(inData.data, g_data[inData.index], URP_DATA_MAX);
			ret = copy_to_user((void *)arg, (const void *)&inData, sizeof(inData));				
			break;
		}

		// BT-2110: write indexed storage
		case URP_STORAGE_WRITE:
		{
			urp_storage inData;

			if (copy_from_user((void *)&inData, (const void *)arg, sizeof(inData)))
				return -EFAULT;
			if (inData.index >= URP_DATA_NUM)
				return -EFAULT;
			memcpy(g_data[inData.index], inData.data, URP_DATA_MAX);
			break;
		}
		
		case URP_BATTERYLV_RTC:
			retval = urp2_RTC_battery_level();
			break;
		case URP_BATTERYLV_SAM:
//			retval = urp2_SAM_battery_level();
			break;
		case URP_IS_DDU:
			retval = urp2_isDDU();		//255: DDU or 0: BCP 
			break;
		case URP_IS_POWERSUPPLY_HEALTHY:
			retval = urp2_is_powersupply_healthy();
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
	if (DEBUG) {
		printk(KERN_INFO "Bind 0x%x to VA 0x%x\n", MAPBASE, (unsigned int)base);
	}
	return 0;
}

static void __init urp2_init_error(void) {
	printk(KERN_INFO "Error registering urp2 device\n");
	iounmap(base);
	unregister_chrdev(URP2_MAJOR, "urp2");
}

static int __init urp2_init(void)
{
	int ret;
	printk(KERN_INFO "URP2 module init - debug mode is %s\n",
            DEBUG ? "enabled" : "disabled");

	ret = register_chrdev(URP2_MAJOR, "urp2", &urp2_fops);
	if (ret < 0) {
		urp2_init_error();
		return ret;
	}
	if (DEBUG) {
		printk(KERN_INFO "URP2: registered module successfully!\n");
	}

	/* Init processing here... */
	ret = addr_init();
	if (ret < 0) {
		urp2_init_error();
		return ret;
	}

	// Init necessary devices
	if (!WITHOUT_SMT) {
		urp2_rf_start();
	}
	return 0;
}


static void __exit urp2_exit(void)
{
	// Remove devices
	if (!WITHOUT_SMT) {
		urp2_rf_stop();
	}

	// iounmap the addr
	iounmap(base);
	unregister_chrdev(URP2_MAJOR, "urp2");
	printk(KERN_INFO "URP2 module exits\n");
}


struct file_operations urp2_fops = {
	owner:   THIS_MODULE,
	read:    urp2_read,
	write:   urp2_write,
	ioctl:   urp2_ioctl,
	open:    urp2_open,
	release: urp2_release,
};

module_init(urp2_init);
module_exit(urp2_exit);

MODULE_AUTHOR(" ");
MODULE_DESCRIPTION("URP2 hardware control module");
MODULE_LICENSE("GPL v2");
