#ifndef __URP2_H__
#define __URP2_H__

//Merged OLD-MAIN: BT-2110
#define URP_DATA_MAX	32
#define URP_DATA_NUM	2

//Merged OLD-MAIN: BT-2110
typedef struct {
	int	index;
	unsigned char data[URP_DATA_MAX];
} urp_storage;

/* URP General */
#define URP_IOCTL_BASE 	'Q'
#define URP_RESET		_IOW(URP_IOCTL_BASE, 0, unsigned char)
#define URP_485			_IOW(URP_IOCTL_BASE, 1, unsigned char)
#define URP_IO			_IOW(URP_IOCTL_BASE, 2, unsigned char)
#define URP_232			_IOW(URP_IOCTL_BASE, 3, unsigned char)
#define URP_FULLMODEM		_IOW(URP_IOCTL_BASE, 4, unsigned char)
#define URP_IRDA		_IOW(URP_IOCTL_BASE, 5, unsigned char)
#define URP_RF			_IOW(URP_IOCTL_BASE, 6, unsigned char)
#define URP_GPRS		_IOW(URP_IOCTL_BASE, 7, unsigned char)
#define URP_BCP_LED		_IOW(URP_IOCTL_BASE, 8, int)
#define URP_DDU_LED		_IOW(URP_IOCTL_BASE, 9, unsigned char)
#define URP_IMMOBILIZER		_IOW(URP_IOCTL_BASE, 10, unsigned char)
#define URP_STORAGE_READ	_IOW(URP_IOCTL_BASE, 11, urp_storage)	//Merged OLD-MAIN: BT-2110
#define URP_STORAGE_WRITE	_IOW(URP_IOCTL_BASE, 12, urp_storage)	//Merged OLD-MAIN: BT-2110
#define URP_BATTERYLV_SAM	_IOW(URP_IOCTL_BASE, 13, unsigned char)
#define URP_BATTERYLV_RTC	_IOW(URP_IOCTL_BASE, 14, unsigned char)
#define URP_IS_DDU		_IOW(URP_IOCTL_BASE, 15, unsigned char)
#define URP_IS_POWERSUPPLY_HEALTHY		_IOW(URP_IOCTL_BASE, 16, unsigned char)

#define URP_485_DIRECT		(0x01)
#define URP_485_REVERSE		(0x02)

#define URP_DEVICE_OFF		(0x00)
#define URP_DEVICE_ON		(0x01)
#define IRDA_UNKNOWN_MODE	(0x02)
#define IRDA_TX_MODE		(0x03)
#define IRDA_RX_MODE		(0x04)

#define URP_BCP_LED1		(0x01)
#define URP_BCP_LED2		(0x02)
#define URP_BCP_LED3		(0x04)
#define URP_BCP_LED4		(0x08)
#define URP_BCP_LEDALL		(0x80)

#define URP_BCP_LED_OFF		(0x00)
#define URP_BCP_LED_ON		(0x01)

#define URP_DDU_LED_OFF		(0x00)
#define URP_DDU_LED_ON		(0x01)

#define URP_IMMOBILIZER_OFF	(0x00)
#define URP_IMMOBILIZER_ON	(0x01)
#define URP_IMMOBILIZER_GET (0x02)	// Mantis 6648

#define URP_BATTERY_GOOD	(0x01)
#define URP_BATTERY_FAIL	(0x00)

/* 4+4 I/O */
#define URP_IO_IN_0		(0x80)
#define URP_IO_IN_1		(0x40)
#define URP_IO_IN_2		(0x20)
#define URP_IO_IN_3		(0x10)
#define URP_IO_OUT_0		(0x08)
#define URP_IO_OUT_1		(0x04)
#define URP_IO_OUT_2		(0x02)
#define URP_IO_OUT_3		(0x01)

typedef struct {
	unsigned char control;
} urp2_io;

#endif // __URP2_H__
