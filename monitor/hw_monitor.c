#include <stdio.h>
#include <pthread.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include "hw_monitor.h"

static int first_line_len(FILE *fp);
static int fgetline(char *line, int max, FILE *fp);
static void cleanup(FILE *fp, char *ptr1, char *ptr2);
static char * get_cpu(void);
static long get_t_jiffies(char *cpu_agg);
static long get_w_jiffies(char *cpu_agg);
static long get_x_contents(char *interface, char *x_bytes);

/* max length of a single line from /proc/meminfo */
#define MAX_STR		30	

/* maximum amount of digits of a memory size (in kB) */
#define MAX_MEM		9

#define NET_PATH	"/sys/class/net/"
#define BASE_ERR_LEN	20
#define BASE_LEN	40

/* first_line_len:	returns the length of the first line in the 
 * *already* opened file stream fp
 */
static int first_line_len(FILE *fp)
{
	int len = 0;
	while (fgetc(fp) != '\n')
		++len;
	rewind(fp);
	return len;

}

/* fgetline:	gets one line from fp and stores it in line */
static int fgetline(char *line, int max, FILE *fp)
{
	if (fgets(line, max, fp) == NULL)
		return -1; 
	return strlen(line);
}

static void cleanup(FILE *fp, char *ptr1, char *ptr2)
{
	fclose(fp);
	free(ptr1);
	free(ptr2);
}

/* returns a pointer to a string containing the first line of /proc/stat 
 */
static char * get_cpu(void) 
{
	FILE *fp;
	char *s;
	int len = 0; 
	if ((fp = fopen("/proc/stat", "r")) == NULL) {
		perror("get_cpu: can't open /proc/stat");
		return NULL; 
	} 

	len = first_line_len(fp);
	s = malloc(sizeof(char) * len + 1);
	if (fgetline(s, len, fp) == -1)
		return NULL;
	fclose(fp);
	return s;
}

/* get_t_jiffies:  given a string cpu_agg in the form
 * cpu user nice system	idle iowait irq	softirq
 * returns user+nice+system+idle+iowait+irq+softirq
 */
static long get_t_jiffies(char *cpu_agg)
{
	char *cpu_agg_cp = malloc(strlen(cpu_agg) + 1);
	strcpy(cpu_agg_cp, cpu_agg);

	long sum = 0;
	char *token = strtok(cpu_agg_cp, " ");
	while ((token = strtok(NULL, " ")))
		sum += atoi(token);
	free(cpu_agg_cp);
	return sum;
	
}

/* get_w_jiffies:  given a string cpu_agg in the form
 * cpu user nice system	idle iowait irq	softirq
 * returns user+nice+system
 */
static long get_w_jiffies(char *cpu_agg) 
{
	char *cpu_agg_cp = malloc(strlen(cpu_agg) + 1);
	strcpy(cpu_agg_cp, cpu_agg);

	int i;
	long sum = 0; 
	char *token = strtok(cpu_agg_cp, " "); 
	for (i = 0; i < 3 && (token = strtok(NULL, " ")); i++)
		sum += atoi(token); 
	free(cpu_agg_cp);
	return sum;
}

/* get_x_contents:	returns the contents of either rx_bytes or
 * tx_bytes as a long on success or -1 on failure
 */
static long get_x_contents(char *interface, char *x_bytes)
{
	FILE *fp;
	char *x_bytes_path, *x_bytes_contents;
	int x_len;
	long nbytes;

	x_bytes_path = malloc(strlen(interface) + 1 + BASE_LEN);
	sprintf(x_bytes_path, "/sys/class/net/%s/statistics/%s", 
		interface, x_bytes);
	if ((fp = fopen(x_bytes_path, "r")) == NULL) {
		fprintf(stderr, "%s: could not open file: %s : %s\n", 
			__func__, x_bytes_path, strerror(errno));
		goto cleanup;
	}

	x_len = first_line_len(fp) + 1;
	x_bytes_contents = malloc(x_len);
	if (fgets(x_bytes_contents, x_len, fp) == NULL) {
		fprintf(stderr, "%s: could not read %s contents\n", 
		__func__, x_bytes_path);
		goto cleanup;
	}

	nbytes = atol(x_bytes_contents);
	cleanup(fp, x_bytes_path, x_bytes_contents);
	return nbytes;

cleanup:
	cleanup(fp, x_bytes_path, x_bytes_contents);
	return -1;
}

/* get_up_speed:	gets the average download speed for the interface
 * and time_delay specified in net_info_struct. Returns a pointer to -1
 * on failure, 0 on success.
 */
void *get_up_speed(void *net_info_struct)
{
	struct net_info *ninfo;
	ninfo = (struct net_info *) net_info_struct;

	char *interface;
	int *result = malloc(sizeof(*result));
	unsigned int time_delay;
	long up_bytes1, up_bytes2;

	*result = -1;

	interface = ninfo->interface;
	time_delay = ninfo->time_delay;

	if ((up_bytes1 = get_x_contents(interface, "tx_bytes")) == -1)
		pthread_exit(result);

	sleep(time_delay);

	if ((up_bytes2 = get_x_contents(interface, "tx_bytes")) == -1)
		pthread_exit(result);

	*result = 0;
	ninfo->speed = (up_bytes2 - up_bytes1) / time_delay;
	pthread_exit(result);
}

/* get_down_speed:	gets the average download speed for the interface
 * and time_delay specified in net_info_struct. Returns a pointer to -1
 * on failure, 0 on success.
 */
void *get_down_speed(void *net_info_struct)
{
	struct net_info *ninfo;
	ninfo = (struct net_info *) net_info_struct;

	char *interface;
	int *result = malloc(sizeof(*result));
	unsigned int time_delay;
	long down_bytes1, down_bytes2;

	*result = -1;

	interface = ninfo->interface;
	time_delay = ninfo->time_delay;

	if ((down_bytes1 = get_x_contents(interface, "rx_bytes")) == -1)
		pthread_exit(result);

	sleep(time_delay);

	if ((down_bytes2 = get_x_contents(interface, "rx_bytes")) == -1)
		pthread_exit(result);

	*result = 0;
	ninfo->speed = (down_bytes2 - down_bytes1) / time_delay;
	pthread_exit(result);
}

/* returns the size of free memory (in MB) */
long get_mem_free(void)
{
	FILE *fp;
	char s1[MAX_STR];
	char *s = s1;		/* makes it easier looping through */
	char free_mem[MAX_MEM];
	int i = 0;

	if ((fp = fopen("/proc/meminfo", "r")) == NULL) {
		perror("get_mem_free: can't open /proc/meminfo");
		return -1; 
	} 

	fgets(s, MAX_STR, fp);
	fgets(s, MAX_STR, fp); 		/* we want second line */

	while (!isdigit(*s))
		++s;
	while (isdigit(*s)) {
		free_mem[i++] = *s++;
	}
	free_mem[i] = '\0';
	fclose(fp);
	return atol(free_mem);
}

/* get_cpu_avg:	gets the average cpu load over the time_delay specified 
 * in cpu_info_struct. Returns a pointer to -1 on failure, 0 on success.
 */
void *get_cpu_avg(void *cpu_info_struct) 
{
	struct cpu_info *cpu_info;
	cpu_info = (struct cpu_info *)cpu_info_struct;

	char *cpu_agg;
	long t_jiffies_1, w_jiffies_1, t_jiffies_2, w_jiffies_2;
	long w_over_period, t_over_period;

	int *result = malloc(sizeof(*result));
	*result = -1;
	
	if ((cpu_agg = get_cpu()) == NULL) 
		pthread_exit(result);

	t_jiffies_1 = get_t_jiffies(cpu_agg);
	w_jiffies_1 = get_w_jiffies(cpu_agg); 

	free(cpu_agg);
	sleep(cpu_info->time_delay);

	if ((cpu_agg = get_cpu()) == NULL) 
		pthread_exit(result);

	t_jiffies_2 = get_t_jiffies(cpu_agg); 
	w_jiffies_2 = get_w_jiffies(cpu_agg);

	w_over_period = w_jiffies_2 - w_jiffies_1;
	t_over_period = t_jiffies_2 - t_jiffies_1;

	free(cpu_agg);

	cpu_info->load_avg = ((double)w_over_period / t_over_period)*100;
	*result = 0;
	pthread_exit(result);
}
