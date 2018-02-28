#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "urp2.h"

int main(int argc, char **argv)
{
	int
		fd,
		rc= 0;

	if (-1 == (fd = open("/dev/urp", O_RDWR)))
	{
		perror("Open failed");
		rc= fd;
		exit(1);
	}
	printf("< %s > SAM Batt. level: %d | RTC Batt. level: %d\n", 
			(ioctl(fd, URP_IS_DDU, 0))?"DDU":"BCP",
			(ioctl(fd, URP_BATTERYLV_SAM, 0)),
			(ioctl(fd, URP_BATTERYLV_RTC, 0)));
	close(fd);
	return(0);
}
