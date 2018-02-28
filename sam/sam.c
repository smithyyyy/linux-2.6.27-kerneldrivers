/*
 *  sam.c
 *
 *  Revision history:
 *  16 May 2008: Initial version
 *  17 Dec 2009: port to at91sam9263 board WongsEVT & WongsDVT
 *  09 Aug 2010: Add support to Device Init Program
 */

// Module header
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/spinlock.h>

// Board specific header
#include <mach/gpio.h>
#include <mach/at91sam9263.h>
#include <mach/at91sam9263_matrix.h>
#include <mach/at91sam9_sdramc.h>
#include <mach/at91_pmc.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include "atmel_serial.h"
#include "sam.h"

// Define SPIN Lock
DEFINE_SPINLOCK(sam_spinlock);

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
#define UART_GET_FIDI(port)	__raw_readl(port + ATMEL_US_FIDI)
#define UART_GET_TTGR(port)	__raw_readl(port + ATMEL_US_TTGR)
#define UART_PUT_FIDI(port,v)	__raw_writel(v, port + ATMEL_US_FIDI)
#define UART_PUT_TTGR(port,v)	__raw_writel(v, port + ATMEL_US_TTGR)

// Device information
#define SAM_MAJOR 148

// Physcial Address of USART 0
#define USART0_PHYADDR AT91SAM9263_BASE_US0

// Size of the address block
#define USART0_SIZE 16384

// MCK - by calculations and the value is proofed by CRO measurement
#define MCK 99000000

/* 7816-3 1997 Table 7, 8 */
static short Ftab[] = { 372, 372, 558,  744, 1116, 1488, 1860, -1, 
                  -1, 512, 768, 1024, 1536, 2048,   -1, -1 };
static short Dtab[] = { -1, 1, 2, 4, 8, 16, 32, 64, 12, 20, -1, -1, 
		  -1, -1, -1, -1 };

// Debug Control
static int DEBUG = 0;
module_param(DEBUG, int, 0);
MODULE_PARM_DESC(DEBUG, "Enable module debug mode.");

// Base Virtual Address for USART0
static void *base;

// To store ATR Answer-to-reset
static unsigned char atr[32];	// Max len of ATR = 32
static unsigned int atrlen;

// File Operations
struct file_operations sam_fops;

// Device Open Count
static int Device_Open = 0;

static int sam_open(struct inode *inode, struct file *file)
{
	if (Device_Open) {
		printk(KERN_INFO "SAM device is busy. Please close another process first\n");
		return -EBUSY;
	}
	Device_Open++;
	try_module_get(THIS_MODULE);
	return 0;
}

static int sam_release(struct inode *inode, struct file *file)
{
	Device_Open--;
	module_put(THIS_MODULE);
	return 0;
}

static ssize_t sam_read(struct file *file, char *buf, size_t count,
                loff_t *ptr)
{
	unsigned long flags;
	unsigned int CSR_status=0;
	unsigned int i;
	int timeout=0;
	char *tmpBuf = (char *)kmalloc(count, GFP_KERNEL);
	int retval=0;

	if (!tmpBuf)
		return -ENOMEM;

	// SPIN Lock to protect reading from SAM
	spin_lock_irqsave(&sam_spinlock, flags);

	if (DEBUG) {
		printk(KERN_INFO "SAM Read count = %d\n", count);
	}
	for (i=0; i<count; i++) {
		timeout = 10000;
		while (!((CSR_status = UART_GET_CSR(base)) & AT91C_US_RXRDY)) {
			timeout--;
			if (timeout == 0) {
			  printk(KERN_INFO "SAM Read Timeout == 0!\n");
			  break;
			}
		}
		tmpBuf[i] = (UART_GET_CHAR(base) & 0x1FF);
		if (DEBUG)
			printk(KERN_INFO "[%.2X]", tmpBuf[i]);
	}
	if (DEBUG) {
		if (timeout == 0)
			printk(KERN_INFO "SAM Read Timeout == 0!\n");
	}
	
	spin_unlock_irqrestore(&sam_spinlock, flags);
	// SPIN Lock end

	// Move Kernel Space Memory to User Space
	retval = copy_to_user(buf, tmpBuf, i);

	// free kernel memory
	kfree(tmpBuf);
	return 0;
}


static ssize_t sam_write(struct file *file, const char *buf,
                size_t count, loff_t * ppos)
{
	unsigned long flags;
	unsigned int CSR_status=0;
	unsigned int i;
	int data;
	char *tmpBuf = (char *)kmalloc(count, GFP_KERNEL);

	if (!tmpBuf) 
		return -ENOMEM;

	// Move User Space Memory to Kernel Space
	if (copy_from_user(tmpBuf, buf, count)) {
		kfree(tmpBuf);
		return -EFAULT;
	}

	// SPIN Lock to protect writing to SAM
	spin_lock_irqsave(&sam_spinlock, flags);

	// Part 1, clear receive buffer
	while (((CSR_status = UART_GET_CSR(base)) & AT91C_US_RXRDY)) {
		data = (UART_GET_CHAR(base) & 0x1FF);
	}

	// Part 2, write to UART's THR
	for (i=0; i<count; i++) {
		// Wait unit TX ready
		while (!((CSR_status = UART_GET_CSR(base)) & AT91C_US_TXRDY));

		// copy buffer
		UART_PUT_CHAR(base, (tmpBuf[i] & 0xFF));

		// Wait unit TX empty
		while (!((CSR_status = UART_GET_CSR(base)) & AT91C_US_TXEMPTY));
	}

	spin_unlock_irqrestore(&sam_spinlock, flags);
	// SPIN Lock end

	// free kernel memory
	kfree(tmpBuf);
	return 0;
}

/*
 * AT91_iso_getchar
 * 
 * Utility to retrieve one char from USART controller
 *
 * by polling UART_CSR register, if ready, get UART_RHR.
 */ 
char AT91_iso_getchar(unsigned int nWait100cycle)
{
	unsigned int CSR_status=0;
	while (!((CSR_status = UART_GET_CSR(base)) & AT91C_US_RXRDY)) {
		if (nWait100cycle>0) {
			udelay(20);	// cycle time??
			nWait100cycle--;
			if (nWait100cycle <= 0) {
				// timeout, break loop
				printk(KERN_INFO "[sam.ko] AT91_iso_getchar(): timeout\n");
				return 0;
			}
		}
	}
	return (UART_GET_CHAR(base) & 0x1FF);
}

void delay400(void)
{
	// Since udelay cannot be so long, 
	// separate the delay into 4
	udelay(100*20);				// keep low a while
	udelay(100*20);				// keep low a while
	udelay(100*20);				// keep low a while
	udelay(100*20);				// keep low a while
	udelay(100*20);
	udelay(100*20);
	udelay(100*20);
	udelay(100*20);
}

/*
 * sam_getATR()
 *
 * Function to retrieve ATR answer-to-reset.
 *
 * Written according to ISO7816-3
 *
 * Structure overview: (byte stream)
 * TS -> T0 -> TA1 -> TB1 -> TC1 -> TD1 -> .... TCK
 * Number of TX(i) is depending on the byte sequence
 */
int sam_getATR(void)
{
	unsigned char TS,T0,TCK;
	unsigned char TA,TB,TC,TD;
	unsigned char TK[16];		// MAX length of TK = 16

	unsigned char Y, K;
	unsigned char TA1=0,FI=0,DI=0;	// TA1 = FI/DI
	unsigned char TC1;		// TC1 = extra guardtime
	unsigned char T;

	unsigned int FIDI=372;

	int i;

	// Reset atrlen
	atrlen = 0;

	// Retrieve TS
	TS = atr[atrlen++] = AT91_iso_getchar(400);
	if (TS != 0x3B && TS != 0x3F) {
		printk(KERN_INFO "[sam.ko] sam_getATR(): ERROR!! TS = %x\n", TS);
		return -1;
	}

	// Retrieve T0
	T0 = atr[atrlen++] = AT91_iso_getchar(400);
	Y = T0 & 0xF0;
	K = T0 & 0x0F;

	// Retrieve TA, TB, TC, TD body
	i=1;
	do {
		if (Y & 0x10) {
			TA = atr[atrlen++] = AT91_iso_getchar(400);
			if (i==1) { 
				// TA1, retrieve FI and DI
				TA1 = TA;
				FI = (TA & 0xF0) >> 4;
				DI = (TA & 0xF);
				FIDI = Ftab[FI] / Dtab[DI];
			}
		}
		if (Y & 0x20) {
			TB = atr[atrlen++] = AT91_iso_getchar(400);
		}
		if (Y & 0x40) {
			TC = atr[atrlen++] = AT91_iso_getchar(400);
			if (i==1) { TC1 = TC; }	// store extra guard time
		}
		if (Y & 0x80) {
			TD = atr[atrlen++] = AT91_iso_getchar(400);
			Y = TD & 0xF0;
			T = TD & 0x0F;
		} else {
			break;
		}
		i++;
	} while (Y != 0);
	
	// Get TKs - The historial characters
	// Begin of Mantis #7157: Invalid Rebate Quota Version checking
	for (i=0; i<K; i++) {
		TK[i] = atr[atrlen++] = AT91_iso_getchar(8000);
	}

	// The check character
	//TCK = atr[atrlen++] = AT91_iso_getchar(400);
	// End of Mantis #7157

// Debug print out
if (DEBUG) {
printk(KERN_INFO "TS = %x\n", TS);
printk(KERN_INFO "T0 = %x\n", T0);
printk(KERN_INFO "Y = 0x%x and K = %d\n", Y, K);
printk(KERN_INFO "TA1 = 0x%x, FI %d, DI %d\n", TA1, FI, DI);
for (i=0; i<K; i++) {
printk(KERN_INFO "TK[%d] = %x\n", i, TK[i]);
}
printk(KERN_INFO "TCK = %x\n", TCK);
printk(KERN_INFO "FI/DI = %d\n", FIDI);
}

	if (FIDI < 0) {
		return -1;
	}

	return FIDI;
}

void USART0_exit(void)
{
	// Reset pins
	at91_set_gpio_input(AT91_PIN_PA26, 0);	// SAM-DATA
	at91_set_gpio_output(AT91_PIN_PB16, 0);	// Reset pin low, return to normal state

	// Disable Clock
	at91_sys_write(AT91_PMC_PCDR, (1 << AT91SAM9263_ID_US0));
}

int sam_chip_init(int samid) {
	int i = 0;

	at91_set_gpio_output(AT91_PIN_PB17, 0);	// set Voltage to 3.3V

	// Power Off All SAMs
	at91_set_gpio_output(AT91_PIN_PB19, 1);	// Chip Select SAM 0
	at91_set_gpio_output(AT91_PIN_PB23, 1);	// Chip Select SAM 1
	at91_set_gpio_output(AT91_PIN_PB27, 1);	// Chip Select SAM 2
	at91_set_gpio_output(AT91_PIN_PB31, 1);	// Chip Select SAM 3
	at91_set_gpio_output(AT91_PIN_PB18, 1);	// PWR0
	at91_set_gpio_output(AT91_PIN_PB22, 1);	// PWR1
	at91_set_gpio_output(AT91_PIN_PB26, 1);	// PWR2
	at91_set_gpio_output(AT91_PIN_PB30, 1);	// PWR3

	at91_set_gpio_output(AT91_PIN_PB16, 0);	// Reset pin
	at91_set_gpio_input(AT91_PIN_PB20, 1);	// RDY pin

	udelay(20);

	// Power On the selected SAM
	// by generating a falling edge to corresponding SAM's PWR
	switch (samid) {
		case 0:
			at91_set_gpio_output(AT91_PIN_PB19, 0);	// Chip Select SAM 0
			at91_set_gpio_output(AT91_PIN_PB18, 0);	// PWR0
			break;
		case 1:
			at91_set_gpio_output(AT91_PIN_PB23, 0);	// Chip Select SAM 1
			at91_set_gpio_output(AT91_PIN_PB22, 0);	// PWR1
			break;
		case 2:
			at91_set_gpio_output(AT91_PIN_PB27, 0);	// Chip Select SAM 2
			at91_set_gpio_output(AT91_PIN_PB26, 0);	// PWR2
			break;
		case 3:
			at91_set_gpio_output(AT91_PIN_PB31, 0);	// Chip Select SAM 3
			at91_set_gpio_output(AT91_PIN_PB30, 0);	// PWR3
			break;
		default:	// samid = 0
			at91_set_gpio_output(AT91_PIN_PB19, 0);	// Chip Select SAM 0
			at91_set_gpio_output(AT91_PIN_PB18, 0);	// PWR0
			break;
	}
	
	// Examine Chip's Ready PIN
	i=0;
	while (at91_get_gpio_value(AT91_PIN_PB20) == 1) {
		i++;
		if (i > 100000) {
			printk(KERN_INFO "SAM(%d) ready timeout\n", samid);
			break;
		}
	}
	return 0;
}

int USART0_init(void)
{
	unsigned int control=0;
	unsigned int mode=0;

	// Set PIO pins
	at91_set_A_periph(AT91_PIN_PA30, 0);	// SAM_CLK, SCK0
	at91_set_A_periph(AT91_PIN_PA26, 0);	// SAM_DATA, TXD0

#if 0
	at91_set_gpio_output(AT91_PIN_PB17, 0);	// set Voltage to 3.3V
	at91_set_gpio_output(AT91_PIN_PB19, 0);	// Chip Select SAM 0
	at91_set_gpio_output(AT91_PIN_PB23, 1);	// Chip Select SAM 1
	at91_set_gpio_output(AT91_PIN_PB27, 1);	// Chip Select SAM 2
	at91_set_gpio_output(AT91_PIN_PB31, 1);	// Chip Select SAM 3
	at91_set_gpio_output(AT91_PIN_PB18, 0);	// PWR0
	at91_set_gpio_output(AT91_PIN_PB22, 1);	// PWR1
	at91_set_gpio_output(AT91_PIN_PB26, 1);	// PWR2
	at91_set_gpio_output(AT91_PIN_PB30, 1);	// PWR3
	at91_set_gpio_output(AT91_PIN_PB16, 0);	// Reset pin
	at91_set_gpio_input(AT91_PIN_PB20, 0);	// RDY pin
#endif

	// Enable Clock
	at91_sys_write(AT91_PMC_PCER, (1 << AT91SAM9263_ID_US0));

	// Disable interrupts
	UART_PUT_IDR(base, (unsigned int)-1);

	// Reset receiver and transmitter
//	control = ATMEL_US_RSTRX | ATMEL_US_RSTTX | ATMEL_US_RXDIS | ATMEL_US_TXDIS;
	control = ATMEL_US_RSTRX | ATMEL_US_RSTTX | ATMEL_US_RXDIS | ATMEL_US_TXDIS | ATMEL_US_RSTSTA | ATMEL_US_RSTIT | ATMEL_US_RSTNACK;
	UART_PUT_CR(base, control);

	// SAM Chip, LTC 1755, init routine
	// default samid = 0
	sam_chip_init(0);

	// Set mode register
	mode = ATMEL_US_USMODE_ISO7816_T0 | 
	       //ATMEL_US_CHRL_8 |
	       //ATMEL_US_PAR_EVEN |
	       ATMEL_US_NBSTOP_1 |
	       ATMEL_US_CLKO |
	       ATMEL_US_USCLKS_MCK | 
	       (3 << 24);		// ATMEL_US_ITER = 3
	UART_PUT_MR(base, mode);

	// preconfiguring FIDI in order to receive ATR
	// SAM_CLK = MCK/CD = 99 MHz / 21  = 4.71 MHz
	// Baud rate = MCK/CD/FIDI
	//
	// thus, set BRGR = CD = 21
	UART_PUT_BRGR(base, 21);

	// FIDI, initial value for SAM communications
	UART_PUT_FIDI(base, 372);
	UART_PUT_TTGR(base, 1);

	// Set control register
	control = ATMEL_US_RXEN | ATMEL_US_TXEN;
	UART_PUT_CR(base, control);

	// set interrupt register
printk(KERN_INFO "KEN.. enabled US RXRDY\n");
//	UART_PUT_IER(base, ATMEL_US_RXRDY | ATMEL_US_TXRDY);
	UART_PUT_IER(base, ATMEL_US_RXRDY);
	return 0;
}

void sam_init_error(void) {
	printk(KERN_INFO "Error registering sam device\n");
	iounmap(base);
	unregister_chrdev(SAM_MAJOR, "sam");
}

/*
 * sam_reset()
 * Purpose: reset sam by adjust PIOs to trigger reset pulse.
 *          [P.S.] Have to be finished within 1 secs
 */
int sam_reset(void)
{
	unsigned int CSR_status=0;
	unsigned int FIDI;

	char temp=0;
	unsigned int ret;

	// IMR
	unsigned long flags;
	unsigned int imr=0;

	if (DEBUG) {
		printk(KERN_INFO "sam_reset Bind 0x%x to VA 0x%x\n", USART0_PHYADDR, (unsigned int)base);
	}

	// Clear PIO signals
	USART0_exit();

	// SPIN Lock to protect reading from SAM
	spin_lock_irqsave(&sam_spinlock, flags);

	// get interrupt register
	imr = UART_GET_IMR(base);
	if (DEBUG) {
		printk(KERN_INFO "Interrupt Register = %d\n", imr);
	}

	// Prepare PIO signals
	ret = USART0_init();

	// set FIDI value to default 372
	UART_PUT_FIDI(base, 372);

	// Retrieve the remaining characters from the stream
	while ( (CSR_status = UART_GET_CSR(base)) & AT91C_US_RXRDY) {
		temp = UART_GET_CHAR(base) & 0x1FF;
	}

	// Generate Reset Signal pattern to SAM card
	//
	//			________
	// ___400 period time___|
	at91_set_gpio_output(AT91_PIN_PB16, 0);	// Reset pin low
	delay400();
	at91_set_gpio_output(AT91_PIN_PB16, 1);	// Reset pin high

	// Start get ATR from SAM card
	FIDI = sam_getATR();
	if (DEBUG) {
		printk(KERN_INFO "FI/DI = %d\n", FIDI);
	}

	if (FIDI == -1) {
		FIDI = 372;
	}

	// Set correct FIDI value from ATR
	UART_PUT_FIDI(base, FIDI);

	// set interrupt register
	UART_PUT_IER(base, imr);
//	UART_PUT_IER(base, ATMEL_US_RXRDY | ATMEL_US_TXRDY);
	if (DEBUG) {
		printk(KERN_INFO "SET Interrupt Register as %d\n", imr);
	}

	spin_unlock_irqrestore(&sam_spinlock, flags);
	// SPIN Lock end

	if (DEBUG) {
		printk(KERN_INFO "SAM Reset Completed\n");
	}
	return 0;
}


int sam_copyATR(ATRStruct *pATR_st) 
{
	unsigned long retval=0;

	printk(KERN_INFO "samid = %d, len = %d(%d), atr = %p\n", 0, pATR_st->len, atrlen, pATR_st->atr);

//	if (atr) {
		if (pATR_st->len >= atrlen) {
			if (!pATR_st->atr)
				return ERR_SAM_INVALID_PARAM;

			retval = copy_to_user(pATR_st->atr, atr, atrlen);
			retval = copy_to_user(&pATR_st->len, &atrlen, sizeof(int));
			retval = 0;
		} else {
			retval = copy_to_user(&pATR_st->len, &atrlen, sizeof(int));
			retval = ERR_SAM_BUFLEN_TOO_SHORT;
		}
//	} else {
//		retval = ERR_SAM_NOATR;
//	}
	return retval;
}

static int sam_ioctl(struct inode *inode, struct file *file,
                unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
		case CMD_SAM_RESET:
			sam_reset();
			return 0;
		case CMD_SAM_SELECT:
			printk(KERN_INFO "Sam Select is selected\n");
			return 0;	// SAM 0 hard coded
		case CMD_SAM_GETATR:
//			sam_getatr((ATRStruct *)arg);
//			return ERR_SAM_BUFLEN_TOO_SHORT;
			return sam_copyATR((ATRStruct *)arg);
		case CMD_SAM_GETSEL:
			printk(KERN_INFO "Sam Getsel is selected\n");
			return 0;
		default:
			printk(KERN_INFO "sam_ioctl: undefined cmd=%d, arg=%ld\n", cmd, arg);
			break;
	}
	return -1;
}

static int __init sam_init(void)
{
	int ret;
	printk(KERN_INFO "Sam module init - debug mode is %s\n",
            DEBUG ? "enabled" : "disabled");

	// Register /dev/sam
	ret = register_chrdev(SAM_MAJOR, "sam", &sam_fops);
	if (ret < 0) {
		sam_init_error();
		return ret;
	}

	if (DEBUG) {
		printk(KERN_INFO "Sam: registered module successfully!\n");
	}

	// Retrieve virtual address of USART0 for controlling it's registers
	base = ioremap(USART0_PHYADDR, USART0_SIZE);
	if (base == NULL) {
		printk(KERN_INFO "Cannot get base address, ioremap error\n");
		return -ENOMEM;
	}
	if (DEBUG) {
		printk(KERN_INFO "Bind 0x%x to VA 0x%x\n", USART0_PHYADDR, (unsigned int)base);
	}

	// Prepare PIO signals
	ret = sam_reset();
	if (ret < 0) {
		sam_init_error();
		return ret;
	}
	return 0;
}

static void __exit sam_exit(void)
{
	at91_set_gpio_input(AT91_PIN_PA26, 0);	// SAM-DATA 
	at91_set_gpio_output(AT91_PIN_PB16, 0);	// Reset pin low

	//gorey added for urp2	
	if (DEBUG) {
		printk(KERN_INFO "PMC_PCSR before:%i\n", (unsigned int)at91_sys_read(AT91_PMC_PCSR));
	}

	// Disable Clock
	at91_sys_write(AT91_PMC_PCDR, (1 << AT91SAM9263_ID_US0));
	
	//gorey added for urp2	
	if (DEBUG) {
		printk(KERN_INFO "PMC_PCSR after:%i\n", (unsigned int)at91_sys_read(AT91_PMC_PCSR));
	}

	// Unregister /dev/sam
	unregister_chrdev(SAM_MAJOR, "sam");

	// iounmap the address
	iounmap(base);
	printk(KERN_INFO "Sam module exits\n");
}


struct file_operations sam_fops = {
	owner:   THIS_MODULE,
	read:    sam_read,
	write:   sam_write,
	ioctl:   sam_ioctl,
	open:    sam_open,
	release: sam_release,
};

module_init(sam_init);
module_exit(sam_exit);


MODULE_AUTHOR(" ");
MODULE_DESCRIPTION("SAM card access module");
MODULE_LICENSE("GPL v2");
