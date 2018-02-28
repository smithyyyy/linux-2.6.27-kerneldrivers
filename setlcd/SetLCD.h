/*
 * SetLCD.h 
 *
 * Linux 2.6.x Driver 
 *
 * Revision history:
 *
 */

#ifndef SETLCD_H
#define SETLCD_H

#define BCP_LCD_OFF	0
#define BCP_LCD_ON	1
#define DDU_LCD_OFF	2
#define DDU_LCD_ON	3

// Command code for IO control
#define SETLCD_IOCTL_BASE		'J'
#define CMD_SET_BRIGHTNESS	_IOW(SETLCD_IOCTL_BASE, 0, int)
#define CMD_SET_CONTRAST	_IOW(SETLCD_IOCTL_BASE, 1, int)
#define CMD_SET_ONOFF		_IOW(SETLCD_IOCTL_BASE, 2, int)
#define CMD_SET_DATA		_IOW(SETLCD_IOCTL_BASE, 3, int)



#endif // SETLCD_H
