#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <linux/input.h>
#include "SetLCD.h"
	
int main(int argc, char **argv)
{	
	int fd, ret, number_input, cmd;

	if (argc<2)
	{
		printf("*************************************\n");
		printf("*	changelcdsettings [cmd] [val]	*\n");
		printf("*	[cmd]:	0=BRIGHTNESS			* \n");
		printf("*			1=CONTRAST				* \n");
		printf("*			2=ONOFF					* \n");
		printf("*			3=DATA					* \n");
		printf("*	[val]:	value					* \n");
		printf("*************************************\n");
		return 0;
	}

	fd = open("/dev/setlcd", O_RDWR);
	if ( fd == -1 ) {
		printf("setlcd open failed\n");
		return (-1);
	}

	cmd = atoi(argv[1]);
	if (cmd==0)
		cmd = CMD_SET_BRIGHTNESS;
	else if (cmd==1)
		cmd = CMD_SET_CONTRAST;
	else if (cmd==2)
		cmd = CMD_SET_ONOFF;
	else if (cmd==3)
		cmd = CMD_SET_DATA;
	
	number_input = atoi(argv[2]);

printf("cmd = %d, number_input = %d\n", cmd, number_input);
	ret = ioctl(fd, cmd, number_input);
	if (ret<0)
	{
		return ret;
	}

	return 0;
}
