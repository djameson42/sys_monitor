#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "hw_monitor.h"

#define TTY_FILE	"/dev/ttyACM0"
#define BUF_SIZE	34	/* 32 spaces in lcd display + '\n' and '\0' */
#define TIME_DELAY	5	/* num secs between each cpu probe */
#define NET_INTERFACE	"wlp8s0"
#define CPU_STR_SIZE	8
#define FMEM_STR_SIZE	8
#define USPEED_STR_SIZE 8
#define DSPEED_STR_SIZE 8
#define BAUD		B9600	

int termios_init(int fd);
char *bytes_to_size(long bytes);

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

	cfsetispeed(&options, BAUD);	
	cfsetospeed(&options, BAUD);	

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

char *bytes_to_size(long bytes)
{
	/* returned value can only be less than or equal to length of 1024 */
	char *ret = malloc(5);

	if (bytes >= 1073741824L)
		sprintf(ret, "%ldGB", bytes / (1024 * 1024 * 1024));
	else if (bytes >= 1048576L)
		sprintf(ret, "%ldMB", bytes / (1024 * 1024));
	else if (bytes >= 1024)
		sprintf(ret, "%ldKB", bytes / 1024);
	else
		sprintf(ret, "%ldB", bytes);
	return ret;
}

int main(void)
{
	int fd;
	long free_mem;
	char buf[BUF_SIZE], cpu_str[CPU_STR_SIZE], fmem_str[FMEM_STR_SIZE],
		uspeed_str[USPEED_STR_SIZE], dspeed_str[DSPEED_STR_SIZE];
	char *uspeed, *dspeed;
	void *uspeed_ret, *dspeed_ret, *cpu_ret;
	struct net_info up_info, down_info;
	struct cpu_info cpu_info;

	pthread_t thread_up;
	pthread_t thread_down;
	pthread_t thread_cpu;

	up_info.interface = NET_INTERFACE;
	up_info.time_delay = TIME_DELAY;

	down_info.interface = NET_INTERFACE;
	down_info.time_delay = TIME_DELAY;

	cpu_info.time_delay = TIME_DELAY;

	fd = open(TTY_FILE, O_RDWR | O_NOCTTY | O_NDELAY);

	if (fd == -1) {
		perror("could not open file: " TTY_FILE);
		exit(EXIT_FAILURE);
	}
	if (termios_init(fd) == -1)
		exit(EXIT_FAILURE);
	while (1) {
		if ((free_mem = get_mem_free()) == -1)
			exit(EXIT_FAILURE);

		pthread_create(&thread_cpu, NULL, get_cpu_avg, &cpu_info);
		pthread_create(&thread_down, NULL, get_down_speed,
				&down_info);
		pthread_create(&thread_up, NULL, get_up_speed, &up_info);

		pthread_join(thread_down, &dspeed_ret);
		pthread_join(thread_up, &uspeed_ret);
		pthread_join(thread_cpu, &cpu_ret);

		if (*(int*)dspeed_ret == -1 || *(int*)uspeed_ret == -1 || 
			*(int*)cpu_ret == -1)
			exit(EXIT_FAILURE);
		free(dspeed_ret);
		free(uspeed_ret);
		free(cpu_ret);
		
		uspeed = bytes_to_size(up_info.speed);
		dspeed = bytes_to_size(down_info.speed);

		sprintf(cpu_str, "%.1f%%", cpu_info.load_avg);
		sprintf(fmem_str, "%ldMB", free_mem/1024);
		sprintf(uspeed_str, "%s/s", uspeed);
		sprintf(dspeed_str, "%s/s", dspeed);

		sprintf(buf, "%-7s%9s\n%-8s%8s", cpu_str, fmem_str, 
			uspeed_str, dspeed_str);
			
		write(fd, buf, strlen(buf) + 1);

		free(uspeed);
		free(dspeed);
	}
	close(fd);
	exit(EXIT_SUCCESS);
}
