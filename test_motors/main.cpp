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
// #include "Controller_test.hpp"


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

    int ret;

    // Catch a Ctrl-C event.
    signal(SIGINT, sigint_handler);
    //Catch segfaults
    signal(SIGSEGV, segfault_handler);

    // Gyem motor is in can0
    CANDevice can0("can0"); 
    can0.begin();
    vector <Gyems> Gyem_Array;
    Gyem_Array.push_back (Gyems(1));
    Gyem_Array.push_back (Gyems(2));
    Gyem_Array.push_back (Gyems(3));
    Gyem_Array.push_back (Gyems(4));
    GyemsManager gyems(Gyem_Array,can0); 
    gyems.begin();
    
    double reference_angular_pos[4] = {0,0,0,0};
    GyemsManager* Gyems_manager = &gyems;

    /* gyems motor initialization */
    Gyems * gyems_1;
    Gyems * gyems_2;
    Gyems * gyems_3;
    Gyems * gyems_4;
    
    gyems_1 = Gyems_manager->getmotor(1); //received_node_id the can_ID set by the Gyems software
    gyems_2 = Gyems_manager->getmotor(2); //received_node_id the can_ID set by the Gyems software
    gyems_3 = Gyems_manager->getmotor(3); //received_node_id the can_ID set by the Gyems software
    gyems_4 = Gyems_manager->getmotor(4); //received_node_id the can_ID set by the Gyems software
    
    Gyems_manager->load_periodic_request(0x92, 0);
    //get initial position reading from motor
    Gyems_manager->request_values(500); 
    usleep(1000);
    Gyems_manager->read_values(); 
    Gyems_manager->request_values(500);
    usleep(1000);
    Gyems_manager->read_values(); 

    /*********************** Incremental mode can not be accumulated with in the loop ***********************/
    reference_angular_pos[0] = static_cast<int>(72000.0f);
    reference_angular_pos[1] = static_cast<int>(72000.0f);
    reference_angular_pos[2] = static_cast<int>(72000.0f);
    reference_angular_pos[3] = static_cast<int>(72000.0f);

    Gyems_manager->pack_position_6_cmd(1, reference_angular_pos[0],360);  // (can node ID, incremental angle in 0.01 degree, max speed in degree/sec)
    Gyems_manager->pack_position_6_cmd(2, reference_angular_pos[1],180);  
    Gyems_manager->pack_position_6_cmd(3, reference_angular_pos[2],180);  
    Gyems_manager->pack_position_6_cmd(4, reference_angular_pos[3],180);  

    while(run_flag){
        /*********************** loop runs in 500Hz ***********************/

        // Gyems_manager->request_values(100);
        // usleep(100);
        // Gyems_manager->read_values(); 

        printf("the reference angular position is %d\n",(reference_angular_pos));

    }
    /************************** END OF RT STUFF **************************/
    /* stop all motors */
    Gyems_manager->pack_torque_cmd(1,0);
    Gyems_manager->pack_torque_cmd(2,0);
    Gyems_manager->pack_torque_cmd(3,0);
    Gyems_manager->pack_torque_cmd(4,0);
    sleep(1);   
    return ret;
}
