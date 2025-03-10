/**********************************************
 * 
 * This programs excutes the imu_task using RT threads
 * 
 * 
 * ********************************************/

#include "SEA_test.hpp"
#include <fstream>
#include <string>
#include <iostream>


using namespace std;
// using namespace LibSerial;

/* global variables for vertical SEA controller */
double estimated_F_int = 0; //estimated interaction force


void *Motors_thread_func(void *data)
{
    /* local variables for chasis controller */
    double reference_angular_pos[4] = {0,0,0,0};
    double BWS_frequency_test = 0.2f;  //Hz
    /* local variables for chasis controller */
    double_t rt_hori_SEA_force[2] = {0,0};
    
    struct period_info pinfo;
    printf("Initializing motor_thread ...\n");
    std::shared_ptr<spdlog::logger> console_logger = spdlog::get("console");
    std::shared_ptr<spdlog::logger> async_logger= spdlog::get("async_file_logger");
    
    const long task_period_ns = (long) (MOTOR_TASK_PERIOD_SECONDS*(1.0e9));
    printf("**********\nStep time is: %ld ns = %f s\n**********\n", task_period_ns, MOTOR_TASK_PERIOD_SECONDS);
    periodic_task_init(&pinfo, task_period_ns);
    static struct timespec loop_start_time;
    static struct timespec loop_end_time;
    static struct timespec tic;
    static struct timespec toc;
    unsigned int overrun_ctr = 0;
    console_logger->info("motor_thread_func Ready.");

    //unpack arguments passed from pthread_create()
    struct argPtr_motors* dataPtr = (struct argPtr_motors*)data;
    GyemsManager* Gyems_manager = dataPtr->arg_gyems;
    WheelsManager* wheels_manager = dataPtr->arg_wheels;
    pthread_mutex_t* mutex = dataPtr->arg_mutex; 
    pthread_cond_t* condition_sent = dataPtr->arg_condition;
    pthread_cond_t* condition_received = dataPtr->arg_condition_2;

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


    /* output the data collected */
    string output_angle = "../data/states_record.txt";
    // string output_angle = "../data/EXP_7July_PID_m30b160_0AP_30BWS.txt";
    ofstream OutFileAngle(output_angle);
    /*********************** End of Initialization Code ***********************/
    clock_gettime(CLOCK_MONOTONIC, &loop_start_time);

    while(run_flag){
        
        clock_gettime(CLOCK_MONOTONIC, &tic);
        double time_stamp = tic2toc(loop_start_time,tic);

        Gyems_manager->request_values(100);
        usleep(100);
        Gyems_manager->read_values(); 


        /* reference signal */
        double omiga = BWS_frequency_test*2.0*3.14159265f;
        
        reference_angular_pos[0] = static_cast<int>(0.0f);   
        reference_angular_pos[1] = static_cast<int>(0.0f);   
        reference_angular_pos[2] = static_cast<int>(0.0f);   
        reference_angular_pos[3] = static_cast<int>(0.0f);   

        Gyems_manager->pack_position_3_cmd(1, reference_angular_pos[0]);  // there can not be two pack_torque_cmd in one loop!!!!
        Gyems_manager->pack_position_3_cmd(2, reference_angular_pos[1]);  // there can not be two pack_torque_cmd in one loop!!!!
        Gyems_manager->pack_position_3_cmd(3, reference_angular_pos[2]);  // there can not be two pack_torque_cmd in one loop!!!!
        Gyems_manager->pack_position_3_cmd(4, reference_angular_pos[3]);  // there can not be two pack_torque_cmd in one loop!!!!
        // Gyems_manager->pack_torque_cmd(0,0);
        // printf("the reference angular position is %d\n",static_cast<int>(reference_angular_pos));
    
        OutFileAngle << reference_angular_pos << "," <<   << "\n";
       
        clock_gettime(CLOCK_MONOTONIC, &toc);
        double delta_t = tic2toc(tic,toc);
        
        
        if (delta_t > MOTOR_TASK_PERIOD_SECONDS) {
            overrun_ctr += 1;
            if (overrun_ctr >= 100)
            {
                run_flag = 0;
            }
            console_logger->warn("Task overrun!!! (motor_thread_func) Loop took {} s. Overrun counter: {}",
                                 delta_t, overrun_ctr);

        }
        
        wait_rest_of_period(&pinfo);
        
    }
    /************************** END OF RT STUFF **************************/
    OutFileAngle.close();
    /* stop all motors */
    Gyems_manager->pack_torque_cmd(0,0);
    console_logger->info("the threat has ended" );
    sleep(1);   
    return NULL;
}

void *sensor_thread_func(void *data)
{
    struct period_info pinfo;
    printf("Initializing motor_thread ...\n");
    std::shared_ptr<spdlog::logger> console_logger = spdlog::get("console");
    std::shared_ptr<spdlog::logger> async_logger= spdlog::get("async_file_logger");
    
    const long task_period_ns = (long) (MOTOR_TASK_PERIOD_SECONDS*(1.0e9));
    printf("**********\nStep time is: %ld ns = %f s\n**********\n", task_period_ns, MOTOR_TASK_PERIOD_SECONDS);
    periodic_task_init(&pinfo, task_period_ns);
    static struct timespec loop_start_time;
    static struct timespec loop_end_time;
    static struct timespec tic;
    static struct timespec toc;
    unsigned int overrun_ctr = 0;
    console_logger->info("sensor_thread_func Ready.");

    //unpack arguments passed from pthread_create()
    struct argPtr_sensor* dataPtr = (struct argPtr_sensor*)data;
    pthread_mutex_t* mutex = dataPtr->arg_mutex; 
    pthread_cond_t* condition_sent = dataPtr->arg_condition;
    pthread_cond_t* condition_received = dataPtr->arg_condition_2;

    // can3.set_timeout(100);
    /*********************** End of Initialization Code ***********************/
    clock_gettime(CLOCK_MONOTONIC, &loop_start_time);
    clock_gettime(CLOCK_MONOTONIC, &tic);

    while(run_flag){
        /*********************** add anything u would like to read here ***********************/
    }
    /************************** END OF RT STUFF **************************/
    return NULL;
}

/******* Real-Time Thread *********/

/******* End Real-Time Thread *********/

double tic2toc(struct timespec tic, struct timespec toc)
{
    double delta_t_s = (double) (toc.tv_sec-tic.tv_sec);
    double delta_t_ns2s = (toc.tv_nsec - tic.tv_nsec)/1000000000.0;
    double delta_t = delta_t_s + delta_t_ns2s;
    return delta_t;
}