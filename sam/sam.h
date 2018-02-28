/*
 *  sam.c
 *
 *  Revision history:
 *  16 May 2008: Initial version
 */
#ifndef __SAM_H__
#define __SAM_H__

#define	SAM0		0x00	// SAM1 on URP
#define	SAM1		0x01	// SAM1 on PB01
#define	SAM2		0x02	// SAM2 on PB01
#define	SAM3		0x03	// SAM3 on PB01
#define	SAM_NONE	0xff	// SAM3 on PB01

typedef struct {
	int len;
	unsigned char *atr;
} ATRStruct;

// Command code for IO control
#define SAM_IOCTL_BASE		'T'
#define CMD_SAM_RESET		_IOW(SAM_IOCTL_BASE, 0, int)
#define CMD_SAM_SELECT		_IOW(SAM_IOCTL_BASE, 1, int)
#define CMD_SAM_GETATR		_IOWR(SAM_IOCTL_BASE, 2, int)
#define CMD_SAM_GETSEL		_IOR(SAM_IOCTL_BASE, 3, int)

#define NUM_OF_SAM	4

// Error code return from SAM module
#define ERR_SAM_RSTFAIL				1
#define ERR_SAM_NOATR				2
#define ERR_SAM_BUFLEN_TOO_SHORT	3
#define ERR_SAM_INVALID_PARAM		4

#define URP_CLK_DIVISOR 12

// SAM USART Interrupts
#define AT91C_US_RXRDY        ( 0x1 <<  0) // (USART) RXRDY Interrupt
#define AT91C_US_TXRDY        ( 0x1 <<  1) // (USART) TXRDY Interrupt
#define AT91C_US_RXBRK        ( 0x1 <<  2) // (USART) Break Received/End of Break
#define AT91C_US_ENDRX        ( 0x1 <<  3) // (USART) End of Receive Transfer Interrupt
#define AT91C_US_ENDTX        ( 0x1 <<  4) // (USART) End of Transmit Interrupt
#define AT91C_US_OVRE         ( 0x1 <<  5) // (USART) Overrun Interrupt
#define AT91C_US_FRAME        ( 0x1 <<  6) // (USART) Framing Error Interrupt
#define AT91C_US_PARE         ( 0x1 <<  7) // (USART) Parity Error Interrupt
#define AT91C_US_TIMEOUT      ( 0x1 <<  8) // (USART) Receiver Time-out
#define AT91C_US_TXEMPTY      ( 0x1 <<  9) // (USART) TXEMPTY Interrupt
#define AT91C_US_ITERATION    ( 0x1 << 10) // (USART) Max number of Repetitions Reached
#define AT91C_US_TXBUFE       ( 0x1 << 11) // (USART) TXBUFE Interrupt
#define AT91C_US_RXBUFF       ( 0x1 << 12) // (USART) RXBUFF Interrupt
#define AT91C_US_NACK         ( 0x1 << 13) // (USART) Non Acknowledge
#define AT91C_US_RIIC         ( 0x1 << 16) // (USART) Ring INdicator Input Change Flag
#define AT91C_US_DSRIC        ( 0x1 << 17) // (USART) Data Set Ready Input Change Flag
#define AT91C_US_DCDIC        ( 0x1 << 18) // (USART) Data Carrier Flag
#define AT91C_US_CTSIC        ( 0x1 << 19) // (USART) Clear To Send Input Change Flag
#define AT91C_US_COMM_TX      ( 0x1 << 30) // (USART) COMM_TX Interrupt
#define AT91C_US_COMM_RX      ( 0x1 << 31) // (USART) COMM_RX Interrupt

#endif
