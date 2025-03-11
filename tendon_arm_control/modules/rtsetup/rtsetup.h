#ifndef RTSETUP_H
#define RTSETUP_H

#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#include <stdlib.h>
#include <unistd.h>
#include <execinfo.h>
#include <sched.h>  
#include <malloc.h>
#include <errno.h>

#define HEAP_TOUCH_SZ (1024 * 1024) // original was 1024*1024
#define handle_error_en(en, msg) \
               do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

struct period_info {
    struct timespec next_period;
    long period_ns;
};

void inc_period(struct period_info *pinfo);
void periodic_task_init(struct period_info *pinfo, long period_in_ns);
void wait_rest_of_period(struct period_info *pinfo);

void segfault_handler(int sig);
void setup_memory();
pthread_attr_t set_attr(int priority);

#endif
