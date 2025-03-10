/**********************************************
 * 
 * This programs excutes the imu_task using RT threads
 * 
 * Public Library Requirement:
 * 
 * 
 * 
 * Private Library Requirement: (modules folder)
 * - socketcan
 * 
 * 
 * Author - Liang Jiale
 * Date   - Jan. 2022
 * 
 * ********************************************/

#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include <ctype.h>
#include <math.h>
#include <cstdlib>
#include <unistd.h>
// C only includes (needs C linking)
extern "C" {
#include "rtsetup.h"
#include "rtutils.h"
}

// C++ includes (needs C++ linking)
#include <vector>
#include <iostream>

//#include "gyems_can_functions.h"
#include "gyemsManager.hpp"

// Logging library.
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
// Task functions
#include "SEA_test.hpp"


#define LOG_Q_SZ 131072
#define RT_PRIO_SENSOR 70
#define RT_PRIO_MOTOR 71


pthread_attr_t t1_attr;
pthread_attr_t t2_attr;
pthread_attr_t t3_attr;
pthread_t thread1;
pthread_t thread2;
pthread_t thread3;
    
int run_flag = 1;
int clear_flag = 0;

struct timespec t_program_start;


// Ctrl-C handler.
void sigint_handler(int dummy){
    // Set the run flag for real-time thread to zero;
    int result;
    run_flag = 0;
    printf("Ctrl-C detected. Stopping real-time thread and exiting.... {}\n");
}

int main(int argc, char* argv[])
{

    clock_gettime(CLOCK_MONOTONIC, &t_program_start);  // time this program starts

    int ret;

    // Catch a Ctrl-C event.
    signal(SIGINT, sigint_handler);
    //Catch segfaults
    signal(SIGSEGV, segfault_handler);
    

    /****************************** SETUP SHARED RESOURCES ******************************/
    
    // Setup a console logger for general logging.
    auto console_logger = spdlog::stdout_color_mt("console");
    auto CONSOLE_LOGGER_LEVEL = spdlog::level::debug;
    auto FILE_LOGGER_LEVEL = spdlog::level::trace;  

    // Async logger for logging to file. 
    // Initialize logging, Setup logging file, Set thread pool
    spdlog::init_thread_pool(LOG_Q_SZ, 1);
    time_t tstart = time(0);
    tm *t0 = localtime(&tstart);
    char log_name[50];
    // sprintf(log_name, "../data_log/data_%04d%02d%02d_%02d%02d%02d.txt", \
	// 	t0->tm_year+1900,t0->tm_mon+1,t0->tm_mday,\
	// 	t0->tm_hour,t0->tm_min,t0->tm_sec);
    auto async_logger = spdlog::create_async_nb<spdlog::sinks::basic_file_sink_mt>("async_file_logger", log_name);
    async_logger->set_level(spdlog::level::trace);
    async_logger->set_pattern("[%H:%M:%S:%f] [%t:%l] {%v}"); // Set logger formatting

    pthread_cond_t cond,cond2; //for signaling between threads 
    pthread_mutex_t mux;        //for locking threads

    if (ret = pthread_mutex_init(&mux, 0)){
        handle_error_en(ret, "pthread_mutex_init");        
    }if (ret = pthread_cond_init(&cond, 0)){
        handle_error_en(ret, "pthread_cond_init");        
    }if (ret = pthread_cond_init(&cond2, 0)){
        handle_error_en(ret, "pthread_cond_init");        
    }


    // Gyem motor is in Can1
    CANDevice can1("can1"); 
    can1.begin();
    vector <Gyems> Gyem_Array;
    Gyem_Array.push_back (Gyems(1));
    Gyem_Array.push_back (Gyems(2));
    Gyem_Array.push_back (Gyems(3));
    Gyem_Array.push_back (Gyems(4));
    motor_Array.reserve(MAX_NO_OF_GYEMS_DEVICES);
    GyemsManager gyems(Gyem_Array,can1); 
    gyems.begin();
    
    
    struct argPtr_motors t1_arg;
    struct argPtr_sensor t3_arg;

    // reserved for future motors thread这取决于线程函数的入参
    t1_arg.arg_gyems = &gyems;
    t1_arg.arg_mutex = &mux;
    t1_arg.arg_condition = &cond;
    t1_arg.arg_condition_2 = &cond2;


    t3_arg.arg_mutex = &mux;
    t3_arg.arg_condition = &cond;
    t3_arg.arg_condition_2 = &cond2;

    /****************************** SETUP SHARED RESOURCES (END) ******************************/

    /****************************** SETUP THREAD ATTRIBUTES ******************************/
    setup_memory(); //这一句要sudo 才能运行
    t1_attr = set_attr(RT_PRIO_MOTOR); // 分配这个任务的优先级
    t2_attr = set_attr(72);
    t3_attr = set_attr(RT_PRIO_SENSOR);


    // changing the stack size based task requirements 我这个应该暂时不需要，如果报了错再说. 
    // default is 16KB, usually not enough and causes "Segmentation fault (core dumped)"
    size_t stack_sz; 
    
    pthread_attr_getstacksize(&t1_attr, &stack_sz);
    console_logger->info("t1_attr original stack size {}", stack_sz);
    stack_sz = 1024*1024*50; 
    pthread_attr_setstacksize(&t1_attr, stack_sz);
    pthread_attr_getstacksize(&t1_attr, &stack_sz);
    console_logger->info("t1_attr new stack size {}", stack_sz);

    pthread_attr_getstacksize(&t2_attr, &stack_sz);
    console_logger->info("t2_attr original stack size {}", stack_sz);
    stack_sz = 1024*1024*5;
    pthread_attr_setstacksize(&t2_attr, stack_sz);
    pthread_attr_getstacksize(&t2_attr, &stack_sz);
    console_logger->info("t2_attr new stack size {}", stack_sz);

    pthread_attr_getstacksize(&t3_attr, &stack_sz);
    console_logger->info("t3_attr original stack size {}", stack_sz);
    stack_sz = 1024*1024*5;
    pthread_attr_setstacksize(&t3_attr, stack_sz);
    pthread_attr_getstacksize(&t3_attr, &stack_sz);
    console_logger->info("t3_attr new stack size {}", stack_sz);
    /****************************** SETUP THREAD ATTRIBUTES (END) ******************************/
    


    /*************** Restrict threads to specific CPU cores      **********************/
    /* this leads to corrupt images. not sure why, better not to use for vision.*/
    // cpu_set_t cpu0, cpu1;
    // CPU_ZERO(&cpu0);
    // CPU_SET(0, &cpu0);
    // pthread_attr_setaffinity_np(&t1_attr, sizeof(cpu_set_t), &cpu0);
    // CPU_ZERO(&cpu1);
    // CPU_SET(1, &cpu1);
    // pthread_attr_setaffinity_np(&t2_attr, sizeof(cpu_set_t), &cpu1);  
    
    /*************** Restrict threads to specific CPU cores (END)     **********************/
    


    /* Create a pthread with specified attributes */
    ret = pthread_create(&thread1, &t1_attr,Motors_thread_func , &t1_arg);
    if (ret){
        handle_error_en(ret, "pthread1_create");        
    }
    usleep(1000);   

    /* Create a pthread with specified attributes */
    ret = pthread_create(&thread3, &t3_attr,sensor_thread_func, &t3_arg);
    if (ret){
        handle_error_en(ret, "pthread3_create"); 
    }


    /* Join the thread and wait until it is done */
    ret = pthread_join(thread1, NULL);
    if (ret){
        handle_error_en(ret, "pthread1_join");
    }

    /* Join the thread and wait until it is done */
    ret = pthread_join(thread3, NULL);
    if (ret){
        handle_error_en(ret, "pthread3_join");
    }

    spdlog::drop_all();
    return ret;
}
