#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "pio.h"

int main(int argc, char **argv)
{
	/* Our file descriptor */
	int fd;
	int rc = 0;

	PIOCstr piostr;
	unsigned int err;
	char cmd;

	/* Open the device */
	fd = open("/dev/pioc", O_RDWR);
	if ( fd == -1 ) {
		perror("open failed");
		rc = fd;
		exit(-1);
	}

	// provide a function to retrieve URP2 device type
	if (argc == 1) {
		piostr.type = 4;
		piostr.pinID = 22;
		rc = ioctl(fd, CMD_PIOC_GET, &piostr);
		if (rc == 0) {
			printf("DDU");
		} else {
			printf("BCP");
		}
		return 0;
	}

	if (argc < 4) {
		printf("%s <A/B/C/D/E> <0-31(PIN)> <G/S/C/I>\n", argv[0]);
		return 0;
	}

	/* IO control */
	if (argc >= 4) {
		if (argv[1][0] == 'A') {
			piostr.type = 0;
		} else if (argv[1][0] == 'B') {
			piostr.type = 1;
		} else if (argv[1][0] == 'C') {
			piostr.type = 2;
		} else if (argv[1][0] == 'D') {
			piostr.type = 3;
		} else if (argv[1][0] == 'E') {
			piostr.type = 4;
		} else {
			return 0;
		}
		piostr.pinID = atoi(argv[2]);
		printf("Base = %s(%d), PID = %d =>", argv[1], piostr.type, piostr.pinID);
		cmd = argv[3][0];
		switch (cmd) {
			case 'G':
				printf(" value = %d\n", ioctl(fd, CMD_PIOC_GET, &piostr));
				break;
			case 'S':
				printf(" set \n");
				err = ioctl(fd, CMD_PIOC_SET, &piostr);
				break;
			case 'C':
				printf(" clear \n");
				err = ioctl(fd, CMD_PIOC_CLEAR, &piostr);
				break;
			case 'I':
				printf(" input mode \n");
				err = ioctl(fd, CMD_PIOC_INPUT, &piostr);
			default:
				break;
		}
	}

	close(fd);
	return 0;
}
