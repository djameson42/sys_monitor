#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include "hw_monitor.h"

#define TTY_FILE	"/dev/ttyACM0"
#define BUF_SIZE	34	/* 32 spaces in lcd display + '\n' and '\0' */
#define TIME_DELAY	5	/* num secs between each cpu probe */

int termios_init(int fd);

/* 
 * initializes serial port 
 *
 * return -1 on failure, 0 otherwise
 */
int termios_init(int fd)
{
	struct termios options;

	if (tcgetattr(fd, &options) == -1) {
		perror("termios_init: ");
		return -1;
	}

	cfsetispeed(&options, B9600);	
	cfsetospeed(&options, B9600);	

	options.c_cflag &= ~PARENB;		
	options.c_cflag &= ~CSTOPB;	
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;	

	options.c_cflag &= ~CRTSCTS;	
	options.c_iflag &= ~(IXON | IXOFF | IXANY);
	options.c_cflag |= (CLOCAL | CREAD);

	/* set to raw mode */
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;

	if (tcsetattr(fd, TCSANOW, &options) == -1) {
		perror("termios_init: ");
		return -1;
	}
	return 0; 
}

int main(void)
{
	int fd;
	long free_mem;
	double load_avg;
	char buf[BUF_SIZE];

	fd = open(TTY_FILE, O_RDWR | O_NOCTTY | O_NDELAY);

	if (fd == -1) {
		perror("could not open file: " TTY_FILE);
		exit(EXIT_FAILURE);
	}
	if (termios_init(fd) == -1)
		exit(EXIT_FAILURE);
	while (1) {
		if ((load_avg = get_cpu_avg(TIME_DELAY)) == -1)
			exit(EXIT_FAILURE);
		if ((free_mem = get_mem_free()) == -1)
			exit(EXIT_FAILURE);

		sprintf(buf, "CPU:%.1f%%\nMEM FREE:%.fMB", load_avg, 
				free_mem/1024.0);
		write(fd, buf, strlen(buf) + 1);
	}

	close(fd);
	exit(EXIT_SUCCESS);
}
