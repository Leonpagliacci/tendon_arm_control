#include "rtsetup.h"

void inc_period(struct period_info *pinfo)
{
    pinfo->next_period.tv_nsec += pinfo->period_ns;

    while (pinfo->next_period.tv_nsec >= 1000000000) {
	/* timespec nsec overflow */
	pinfo->next_period.tv_sec++;
	pinfo->next_period.tv_nsec -= 1000000000;
    }
}

void periodic_task_init(struct period_info *pinfo, long period_in_ns)
{   
    /* for simplicity, hardcoding a 1ms period */
    pinfo->period_ns = period_in_ns;

    clock_gettime(CLOCK_MONOTONIC, &(pinfo->next_period));
}

void wait_rest_of_period(struct period_info *pinfo)
{
    inc_period(pinfo);

    /* for simplicity, ignoring possibilities of signal wakes */
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &pinfo->next_period, NULL);
}

void segfault_handler(int sig)
{
    void *array[128];
    size_t size;

    size = backtrace(array, 10);
    
    fprintf(stderr, "ERROR: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(-1);
}

void setup_memory(){
    /* Extra heap size intended to cover all dynamic memory this process
    * might use.
    * If the process allocates more than this limit, there is a risk of
    * page faults.
    */
    
    /* Lock memory */
    
    int mret;
    printf("Locking memory.\n");
    mret=mlockall(MCL_CURRENT|MCL_FUTURE);
    
    if (mret != 0){
        handle_error_en(mret, "mlockall");
    }

    /* Turn off malloc trimming */
    mallopt(M_TRIM_THRESHOLD, -1);

    /* Turn off malloc mmap usage */
    mallopt(M_MMAP_MAX, 0);

    int page_size = sysconf(_SC_PAGESIZE);
	char *buf = (char *) malloc(HEAP_TOUCH_SZ);

    /* Touch each memory page in order to make sure we get all page faults
	 * now and not later on during execution
	 */
	for (int i = 0; i < HEAP_TOUCH_SZ; i += page_size)
		buf[i] = 0;

	free(buf);
    printf("memory locked.\n");

}

pthread_attr_t set_attr(int priority){
    
    struct sched_param param;
    pthread_attr_t attr;
    int ret;

    /* Initialize pthread attributes (default values) */
    ret = pthread_attr_init(&attr);
    if (ret){
        handle_error_en(ret, "pthread_attr_init");
    }

    /* Set a specific stack size  */
    ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
    if (ret){
        handle_error_en(ret, "pthread_attr_setstacksize");
    }

    /* Set scheduler policy and priority of pthread */
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (ret){
        handle_error_en(ret, "pthread_attr_setschedpolicy");
    }
 
    param.sched_priority = priority;

    ret = pthread_attr_setschedparam(&attr, &param);
    if (ret){
        handle_error_en(ret, "pthread_attr_setschedparam");
    }

    /* Use scheduling parameters of attr */
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (ret){
        handle_error_en(ret, "pthread_attr_setinheritsched");
    }

    
    return attr;
}