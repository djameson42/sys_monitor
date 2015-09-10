#ifndef HW_MONITOR_H
#define HW_MONITOR_H

struct net_info {
	char *interface;
	unsigned int time_delay;
	long speed;
};

struct cpu_info {
	unsigned int time_delay;
	double load_avg;
};

void *get_up_speed(void *net_info_struct);
void *get_down_speed(void *net_info_struct);
void *get_cpu_avg(void *time);
long get_mem_free(void);


#endif
