#ifndef _GYEMS_H_
#define _GYEMS_H_

#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h> 
#include <pthread.h>
#include <assert.h>

#include "SocketCAN.hpp"
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <math.h>
#include <string.h>
#include <vector>
#include <array>
#include <algorithm>
#include <cstdlib>



//#define DEBUG 

struct PIDconstant{
    uint8_t anglePidKp =0;
    uint8_t anglePidKi=0;
    uint8_t speedPidKp=0;
    uint8_t speedPidKi=0;
    uint8_t iqPidKp=0;
    uint8_t iqPidKi=0;
};

class Gyems {
public:
    Gyems(uint8_t nodeId);

    //uint8_t getNodeId(); 
    uint8_t node_id;
    uint16_t position;
    int64_t multi_turn_position;
    int16_t voltage,torque,speed,phase_ai, phase_bi,phase_ci;
    uint8_t temp,error_state;
    uint16_t raw_position,encoderoffset;
    int32_t accel_data;
    PIDconstant pid;
private:
};

#endif