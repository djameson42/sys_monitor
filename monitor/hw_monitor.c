#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "hw_monitor.h"

static int fgetline(char *line, int max, FILE *fp);
static char * get_cpu(void);
static long get_t_jiffies(char *cpu_agg);
static long get_w_jiffies(char *cpu_agg);

/* max length of a single line from /proc/meminfo */
#define MAX_STR		30	

/* maximum amount of digits of a memory size (in kB) */
#define MAX_MEM		9


/* fgetline:	gets one line from fp and stores it in line */
static int fgetline(char *line, int max, FILE *fp)
{
	if (fgets(line, max, fp) == NULL)
		return -1; 
	return strlen(line);
}

/* returns a pointer to a string containing the first line of /proc/stat */
static char * get_cpu(void) 
{
	FILE *fp;
	char *s;
	int len = 0; 
	if ((fp = fopen("/proc/stat", "r")) == NULL) {
		perror("get_cpu: can't open /proc/stat");
		return NULL; 
	} 

	while (fgetc(fp) != '\n')
		++len;
	s = malloc(sizeof(char) * len + 1);
	if (fgetline(s, len, fp) == -1)
		return NULL;
	fclose(fp);
	return s;
}

/* get_t_jiffies:  given a string cpu_agg in the form (fields are in jiffies)
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

/* get_w_jiffies:  given a string cpu_agg in the form (fields are in jiffies)
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

/* returns the size of free memory (in mB) */
long get_mem_free(void)
{
	FILE *fp;
	char s1[MAX_STR];
	char *s = s1;			/* makes it easier looping through */
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

/*
 * returns the average cpu load as a percentage over t seconds. 
 */

double get_cpu_avg(unsigned int t) 
{
	char *cpu_agg;
	long t_jiffies_1, w_jiffies_1, t_jiffies_2, w_jiffies_2;
	long w_over_period, t_over_period;

	if ((cpu_agg = get_cpu()) == NULL) 
		return -1.0; 

	t_jiffies_1 = get_t_jiffies(cpu_agg);
	w_jiffies_1 = get_w_jiffies(cpu_agg); 

	free(cpu_agg);
	sleep(t);

	if ((cpu_agg = get_cpu()) == NULL) 
		return -1.0;

	t_jiffies_2 = get_t_jiffies(cpu_agg); 
	w_jiffies_2 = get_w_jiffies(cpu_agg);

	w_over_period = w_jiffies_2 - w_jiffies_1;
	t_over_period = t_jiffies_2 - t_jiffies_1;

	free(cpu_agg);

	return ((double)w_over_period / t_over_period) * 100; 
}
