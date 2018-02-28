#ifndef __PIO_H__
#define __PIO_H__

typedef struct {
	int type;
	int pinID;
} PIOCstr;

// Command code for IO control
#define PIOC_IOCTL_BASE		'H'
#define CMD_PIOC_GET		_IOW(PIOC_IOCTL_BASE, 0, int)
#define CMD_PIOC_SET		_IOW(PIOC_IOCTL_BASE, 1, int)
#define CMD_PIOC_CLEAR		_IOW(PIOC_IOCTL_BASE, 2, int)
#define CMD_PIOC_INPUT		_IOW(PIOC_IOCTL_BASE, 3, int)

#endif
