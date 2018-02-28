/*
 * touchkey.h 
 *
 * Revision history:
 * inital version
 */

#ifndef TOUHEKEY_H
#define TOUHEKEY_H

#define NO_KEY_EVENT            0
#define KEY_PRESS_EVENT         1
#define KEY_RELEASE_EVENT       2

#define TOUCHKEY_TYPE_BCP	2
#define TOUCHKEY_TYPE_DDU	1

typedef	enum {
	BCP_ID = 21,
	DDU_ID = 22,
}KeypadID;

typedef struct {
        unsigned char keytype;
        unsigned int keycode;
        unsigned long duration;
}key_info;

// Command code for IO control
#define TOUCH_IOCTL_BASE		 'I'
#define CMD_TOUCHKEY_INIT       _IOW(TOUCH_IOCTL_BASE, 0, int)
#define CMD_TOUCH_STARTREAD     _IOW(TOUCH_IOCTL_BASE, 1, int)
#define CMD_TOUCH_STOPREAD      _IOW(TOUCH_IOCTL_BASE, 2, int)
#define CMD_TOUCH_GETKEY        _IOW(TOUCH_IOCTL_BASE, 3, int)

#endif // CY8C22545_H
