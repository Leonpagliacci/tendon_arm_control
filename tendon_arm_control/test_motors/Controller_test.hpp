#ifndef SEA_TEST_H
#define SEA_TEST_H

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
// #include "atidaq_funcs.h"
#include "rtsetup.h"
#include "rtutils.h"
}

// C++ includes (needs C++ linking)
#include <iostream>

// Logging library.
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"


#include "gyems.hpp"
#include "gyemsManager.hpp"


#define MOTOR_TASK_PERIOD_SECONDS 0.002
#define SENSOR_TASK_PERIOD_SECONDS 0.002
#define LEAD 2 //(mm)

#define SPR_ENC_GAIN 0.001
#define MENC_TO_FLOAT(_enc) (_enc)*MENC_GAIN
#define SPR_ENC_TO_FLOAT(_enc) (_enc)*SPR_ENC_GAIN
#define NUM_FORCE_CHAN 6
#define MAX_SENSOR_READ_BUFFER 48



#define SPDLOG_VAR(var_) async_logger->trace("{:s}: {:.7e}", #var_, var_);

// static double loop_usage_array[(long)(1.0/TASK_PERIOD_SECONDS)];

extern int run_flag;
extern int clear_flag;
extern struct timespec t_program_start;

//put all the arguments you want to pass to the function here
struct argPtr_motors{
        pthread_cond_t * arg_condition; 
        pthread_cond_t * arg_condition_2; 
        pthread_mutex_t * arg_mutex;
        GyemsManager * arg_gyems;
        };
// struct argPtr_gyems{
//         pthread_cond_t * arg_condition; 
//         pthread_cond_t * arg_condition_2; 
//         pthread_mutex_t * arg_mutex;
//         GyemsManager * arg_motor;
//         };
struct argPtr_sensor{
        pthread_cond_t * arg_condition; 
        pthread_cond_t * arg_condition_2; 
        pthread_mutex_t * arg_mutex;
        };   

/******* Real-Time Thread *********/
void *Motors_thread_func(void *data);
void *sensor_thread_func(void *data);



double tic2toc(struct timespec tic, struct timespec toc);

#endif