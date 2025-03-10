#include "gyemsManager.hpp"
/*****************************
 * Library for control of GYEMS motors 
 * V1.0 by Chang Hong - Dec 2020
 * ******************************/

int GyemsManager::begin()
{
    no_of_motor = gyems_array.size();
    create_lookup();
    return 0;
}

int GyemsManager::isGyemsId(int canId){
    if ((canId&0x1c0)==0x140)
        return 1;
    else
        return 0;
}

int GyemsManager::getsize(){
    return no_of_motor;
}

void GyemsManager::create_lookup()
{
    for (int i =0; i<no_of_motor; i++){
        lookup[i] = gyems_array[i].node_id;
    }
}


int GyemsManager::getmotorindex(uint32_t received_node_id)
{
    int i;
    for (i =0; i<no_of_motor; i++){
        if (lookup[i]  == received_node_id)
            return i;
    }
    return i;
}

Gyems* GyemsManager::getmotor(uint32_t received_node_id)
{
    int i;
    for (i =0; i<no_of_motor; i++){
        if (lookup[i]  == received_node_id)
            return &gyems_array[i];;
    }
    return NULL;
}


int GyemsManager::load_periodic_request(uint32_t desired_values, uint32_t node_id)
{
    if(requestlen < MAX_GYEMS_REQUESTS){
        
        requests[0][requestlen] = GET_GYEMSCANID(node_id);
        requests[1][requestlen] = desired_values;
        requestlen++;
        return 0;
    }
    else{
        printf("Number of request exceed");
        return -1;
    }
}

int GyemsManager::request_values(uint16_t delay)
{
    //for current iteration. recommeneded delay 1000us btween requests
    //observed: if request for #02E and #031 without sufficient delay, imu will reply to #02E twice instead
    int retval = 0;
    tx_buf.can_dlc = 8;
    // Send all the requests.
    for(int i = 0; i<requestlen; i++) {
        memset(tx_buf.data,0,8);
        tx_buf.can_id = requests[0][i];
        tx_buf.data[0] = requests[1][i]; //左边4bit，右边32bit，不会截断么？
        retval += channel_name.write(tx_buf); //track number of successful msg   
        usleep(delay);
    }
    return retval;
}

int GyemsManager::read_values() {
    unsigned int read_ctr = 0;
    
    while(channel_name.available()&&read_ctr<MAX_READ_BUFFER){
        channel_name.read(rx_frames[read_ctr]);
        read_ctr++;
    }    //这个read函数竟然没在一个单独的线程里？buffer满了之后咋办，剩下的消息就直接不接收了，但是这个buffer也不能太大，不然一直在读  //看底层的机制，socket应该是读了之后才会清，不然一直存在一个数据结构里的
    // Update the values in the struct based on each can message
    for (unsigned int i=0; i < read_ctr; i++) {
        parse_can_frame_and_update_signals(rx_frames[i]);
    }

    return read_ctr;
}


int GyemsManager::parse_can_frame_and_update_signals(struct can_frame &frame)
{   
    int index=getmotorindex(frame.can_id & 0x3f);
    
    //Note: IF satements are used to filter out the request from the reply. 
    switch(frame.data[0]){
        case  0x9A:
            status_1_reply(&frame,&gyems_array[index].temp,&gyems_array[index].voltage,&gyems_array[index].error_state);
            break;
        case 0xA1:
        case 0xA2:
        case 0xA3:
        case 0xA4:
        case 0x9C:
            if(frame.data[1]!=0)
                unpack_speed_torque_reply(&frame,&gyems_array[index].temp,&gyems_array[index].torque,&gyems_array[index].speed,&gyems_array[index].position);
            break;

        case 0xA5:
        case 0xA6:
            if(!(frame.data[1]==1 || frame.data[1]==0))
                unpack_speed_torque_reply(&frame,&gyems_array[index].temp,&gyems_array[index].torque,&gyems_array[index].speed,&gyems_array[index].position);
            break;

        case 0x9D:
            if(!(frame.data[6]==0 && frame.data[7]==0))
                status_3_reply(&frame,&gyems_array[index].phase_ai,&gyems_array[index].phase_bi,&gyems_array[index].phase_ci);
                break;
        case 0x94:
            //if(!(frame.data[6]==0 && frame.data[7]==0))
                unpack_single_turn_angle(&frame,&gyems_array[index].position);
                
            break;
        case 0x92:
            //if(!(frame.data[1]==0 && frame.data[2]==0))
                unpack_multi_turn_angle(&frame,&gyems_array[index].multi_turn_position);
            break;
        case 0x90:
            //if(!(frame.data[2]==0 && frame.data[3]==0))
                unpack_read_encoder(&frame,&gyems_array[index].position,&gyems_array[index].raw_position,&gyems_array[index].encoderoffset);
            break;
        case 0x30:
                unpack_read_PID(&frame,&gyems_array[index].pid);
            break;
        case 0x33:
                unpack_read_acceleration(&frame,&gyems_array[index].accel_data);
            break;
        default:
            printf(" Motor %x: unknown ID: %x\n",frame.can_id,frame.data[0]);

    }
    return (frame.can_id & 63); //returns node number where data comes from 
}

void GyemsManager::pack_torque_cmd(int nodeID, int16_t iqControl){
     //range :-2000~2000, corresponding to the actual torque current range -32A~32A
     /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0xA1;                                              
     tx_buf.data[1] = 0;                
     tx_buf.data[2] = 0;                   
     tx_buf.data[3] = 0; 
     tx_buf.data[4] = iqControl&0xff;                
     tx_buf.data[5] = (iqControl>>8)&0xff;                 
     tx_buf.data[6] = 0;  
     tx_buf.data[7] = 0;                
    
    //printf("writing: %x %x %x %x (%x %x) %x %x ", tx_buf.data[0],tx_buf.data[1],tx_buf.data[2],tx_buf.data[3],tx_buf.data[4],tx_buf.data[5],tx_buf.data[6],tx_buf.data[7]);
    channel_name.write(tx_buf);
    }

void GyemsManager::pack_speed_cmd(int nodeID, int32_t speed){
     
    //actual speed of 0.01dps/LSB
    //36000 represents 360°

     /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0xA2;                                 
     tx_buf.data[1] = 0;                 
     tx_buf.data[2] = 0;                   
     tx_buf.data[3] = 0;  
     tx_buf.data[4] = speed&0xff;                
     tx_buf.data[5] = (speed>>8)&0xff;                  
     tx_buf.data[6] = (speed>>16)&0xff;  
     tx_buf.data[7] = (speed>>24)&0xff;                  
    
    //printf("Sent Error %x %x %x %x %x %x %x %x\n",tx_buf.data[0],tx_buf.data[1],rx_buf->data[2],rx_buf->data[3],rx_buf->data[4],rx_buf->data[5],rx_buf->data[6],rx_buf->data[7]);    
    channel_name.write(tx_buf);
    }

void GyemsManager::unpack_speed_torque_reply(struct can_frame* rx_msg, uint8_t* temp, int16_t* ptorque,int16_t* pspeed,uint16_t* pposition){
//1. Motor temperature（ 1℃/LSB）
//2. Motor torque current(Iq)（Range:-2048~2048,real torque current range:-33A~33A）
//3. Motor speed（1dps/LSB）
//4. Encoder position value（14bit encoder value range 0~16383）
    *temp = rx_msg->data[1];

    *ptorque = (rx_msg->data[3]<<8)|rx_msg->data[2];
    *pspeed = (rx_msg->data[5]<<8)|(rx_msg->data[4]);
    *pposition = (rx_msg->data[7]<<8)|rx_msg->data[6];       
    #ifdef DEBUG_GYEMS
    printf("Temp: %hu toruqe: %d speed: %d pos: %u \n", *temp, *ptorque, *pspeed, *pposition);
    #endif    
}
 

void GyemsManager::pack_off_cmd(int nodeID){
     
     /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0x80;                                            
     tx_buf.data[1] = 0;                
     tx_buf.data[2] = 0;                  
     tx_buf.data[3] = 0;  
     tx_buf.data[4] = 0;            
     tx_buf.data[5] = 0;               
     tx_buf.data[6] = 0;  
     tx_buf.data[7] = 0;                 
    
    channel_name.write(tx_buf);
    }

void GyemsManager::pack_stop_cmd(int nodeID){
     
    /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0x81;                                            
     tx_buf.data[1] = 0;                
     tx_buf.data[2] = 0;                  
     tx_buf.data[3] = 0;  
     tx_buf.data[4] = 0;            
     tx_buf.data[5] = 0;               
     tx_buf.data[6] = 0;  
     tx_buf.data[7] = 0;    
     
    channel_name.write(tx_buf);
    }

void GyemsManager::pack_run_cmd(int nodeID){
     
    /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0x88;                                            
     tx_buf.data[1] = 0;                
     tx_buf.data[2] = 0;                  
     tx_buf.data[3] = 0;  
     tx_buf.data[4] = 0;            
     tx_buf.data[5] = 0;               
     tx_buf.data[6] = 0;  
     tx_buf.data[7] = 0;    
     
    channel_name.write(tx_buf);
}
void GyemsManager::clear_motor_error(int nodeID){
     
    /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0x9B;                                            
     tx_buf.data[1] = 0;                
     tx_buf.data[2] = 0;                  
     tx_buf.data[3] = 0;  
     tx_buf.data[4] = 0;            
     tx_buf.data[5] = 0;               
     tx_buf.data[6] = 0;  
     tx_buf.data[7] = 0;    
     
    channel_name.write(tx_buf);
}

void GyemsManager::read_status_1_error(int nodeID){
     
    /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0x9A;                                            
     tx_buf.data[1] = 0;                
     tx_buf.data[2] = 0;                  
     tx_buf.data[3] = 0;  
     tx_buf.data[4] = 0;            
     tx_buf.data[5] = 0;               
     tx_buf.data[6] = 0;  
     tx_buf.data[7] = 0;    
     
    channel_name.write(tx_buf);
}

void GyemsManager::status_1_reply(struct can_frame* rx_msg, uint8_t* temp, int16_t* pvoltage,uint8_t* perror_state){
    //NOTE: error flag cannot be cleared when the motor status does not return to normal
    *temp = rx_msg->data[1];
    *pvoltage = (rx_msg->data[4]<<8)|rx_msg->data[3];
    *perror_state = (rx_msg->data[7]);

    #ifdef DEBUG_GYEMS 
    printf("Voltage: %d\n",*pvoltage);
    #endif

    switch (*perror_state) {
        case 1:
            printf("Error: Low voltage protection\n");
            break;
        case 8:
            printf("Overtemperature protection\n");
            break;
        case 0:
            break;
        default:
            printf("Unknown error\n");
            break;
    }   
}

void GyemsManager::read_status_2_data(int nodeID){
     
    /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0x9C;                                            
     tx_buf.data[1] = 0;                
     tx_buf.data[2] = 0;                  
     tx_buf.data[3] = 0;  
     tx_buf.data[4] = 0;            
     tx_buf.data[5] = 0;               
     tx_buf.data[6] = 0;  
     tx_buf.data[7] = 0;    
     
    channel_name.write(tx_buf);
}

void GyemsManager::read_status_3_phase_current(int nodeID){
     
    /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0x9D;                                            
     tx_buf.data[1] = 0;                
     tx_buf.data[2] = 0;                  
     tx_buf.data[3] = 0;  
     tx_buf.data[4] = 0;            
     tx_buf.data[5] = 0;               
     tx_buf.data[6] = 0;  
     tx_buf.data[7] = 0;    
     
    channel_name.write(tx_buf);
}

void GyemsManager::status_3_reply(struct can_frame* rx_msg, int16_t* pca,int16_t* pcb,int16_t* pcc){

    *pca = (rx_msg->data[3]<<8)|rx_msg->data[2];
    *pcb = (rx_msg->data[5]<<8)|(rx_msg->data[4]);
    *pcc = (rx_msg->data[7]<<8)|rx_msg->data[6];
    
    #ifdef DEBUG_GYEMS   
    printf("Error: phase current a: %d phase current b: %d phase current c: %d \n", *pca, *pcb, *pcc);
     #endif
}

void GyemsManager::pack_multi_torque_cmd(int nodeID, int16_t iqControl, int16_t iqControl2, int16_t iqControl3,int16_t iqControl4){
     
     /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x280+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = iqControl&0xff;                                              
     tx_buf.data[1] = (iqControl>>8)&0xff;                
     tx_buf.data[2] = iqControl2&0xff;                                              
     tx_buf.data[3] = (iqControl2>>8)&0xff;                
     tx_buf.data[4] = iqControl3&0xff;                   
     tx_buf.data[5] = (iqControl3>>8)&0xff;                 
     tx_buf.data[6] = iqControl4&0xff;                                              
     tx_buf.data[7] = (iqControl4>>8)&0xff;                
    
    channel_name.write(tx_buf);
    }

void GyemsManager::pack_position_1_cmd(int nodeID, int32_t angle){
     //multi-turn
    //actual position is 0.01degree/LSB, 36000 represents 360°
     #ifdef DEBUG_GYEMS   
    printf("Error: angle: %ld \n", angle);
     #endif

     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0xA3;                                 
     tx_buf.data[1] = 0;                 
     tx_buf.data[2] = 0;                   
     tx_buf.data[3] = 0;  
     tx_buf.data[4] = angle&0xff;                
     tx_buf.data[5] = (angle>>8)&0xff;                  
     tx_buf.data[6] = (angle>>16)&0xff;  
     tx_buf.data[7] = (angle>>24)&0xff;                  
     
    channel_name.write(tx_buf);
}

void GyemsManager::pack_position_2_cmd(int nodeID, int32_t angle, uint16_t max_speed){
     //multi-turn
    //actual position is 0.01degree/LSB, 36000 represents 360°
    //actual speed of 1dps/LSB
    #ifdef DEBUG_GYEMS   
    printf("Error: angle: %ld speed: %u \n", angle, max_speed);
     #endif

     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0xA4;                                 
     tx_buf.data[1] = 0;                 
     tx_buf.data[2] = max_speed&0xff;                   
     tx_buf.data[3] = (max_speed>>8)&0xff;  
     tx_buf.data[4] = angle&0xff;                
     tx_buf.data[5] = (angle>>8)&0xff;                  
     tx_buf.data[6] = (angle>>16)&0xff;  
     tx_buf.data[7] = (angle>>24)&0xff;                  
    
    channel_name.write(tx_buf);
}

void GyemsManager::pack_position_3_cmd(int nodeID, uint16_t angle, uint8_t spinDirection){
    //single turn

    //actual position is 0.01degree/LSB, the actual angle range is 0°~359.99 . ie. range 0~35999. 
    //0x00 for clockwise and 0x01 for counterclockwise

    #ifdef DEBUG_GYEMS   
    printf("Error: angle: %u spinDirection: %hu\n", angle, spinDirection);
     #endif
     /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0xA5;                                 
     tx_buf.data[1] = spinDirection;                 
     tx_buf.data[2] = 0;                   
     tx_buf.data[3] = 0;  
     tx_buf.data[4] = angle&0xff;                
     tx_buf.data[5] = (angle>>8)&0xff;                  
     tx_buf.data[6] = 0;  
     tx_buf.data[7] = 0;                  
     
    channel_name.write(tx_buf);
}

void GyemsManager::pack_position_4_cmd(int nodeID, uint16_t angle, uint16_t max_speed, uint8_t spinDirection){
     //single turn

    //actual position is 0.01degree/LSB, the actual angle range is 0°~359.99 . ie. range 0~35999. 
    //actual speed of 1dps/LSB.
    //0x00 for clockwise and 0x01 for counterclockwise

    #ifdef DEBUG_GYEMS   
    printf("Error: angle: %u max_speed: %u spinDirection: %hu \n", angle, max_speed,spinDirection);
     #endif
     /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0xA6;                                 
     tx_buf.data[1] = spinDirection;  
     tx_buf.data[2] = max_speed&0xff;                
     tx_buf.data[3] = (max_speed>>8)&0xff;  
     tx_buf.data[4] = angle&0xff;                
     tx_buf.data[5] =   (angle>>8)&0xff;                
     tx_buf.data[6] = 0;  
     tx_buf.data[7] = 0;                  
     
    channel_name.write(tx_buf);
}


void GyemsManager::read_single_turn_angle(int nodeID){
     
    /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0x94;                                            
     tx_buf.data[1] = 0;                
     tx_buf.data[2] = 0;                  
     tx_buf.data[3] = 0;  
     tx_buf.data[4] = 0;            
     tx_buf.data[5] = 0;               
     tx_buf.data[6] = 0;  
     tx_buf.data[7] = 0;    
     
    channel_name.write(tx_buf);
}

void GyemsManager::unpack_single_turn_angle(struct can_frame* rx_msg, uint16_t* pangle){
    
    *pangle = (rx_msg->data[7]<<8)|rx_msg->data[6];
    #ifdef DEBUG_GYEMS   
    printf("Position angle %u \n", *pangle);
     #endif
        
}

void GyemsManager::read_multi_turn_angle(int nodeID){
     
    /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0x92;                                            
     tx_buf.data[1] = 0;                
     tx_buf.data[2] = 0;                  
     tx_buf.data[3] = 0;  
     tx_buf.data[4] = 0;            
     tx_buf.data[5] = 0;               
     tx_buf.data[6] = 0;  
     tx_buf.data[7] = 0;    
     
    channel_name.write(tx_buf);
}

void GyemsManager::unpack_multi_turn_angle(struct can_frame* rx_msg, int64_t* mangle){
    
    *mangle = (uint64_t(rx_msg->data[7])<<48)|(uint64_t(rx_msg->data[6])<<40)|(uint64_t(rx_msg->data[5])<<32)|(uint64_t(rx_msg->data[4])<<24)|(uint64_t(rx_msg->data[3])<<16)|(uint64_t(rx_msg->data[2])<<8)|rx_msg->data[1];
    
    #ifdef DEBUG_GYEMS 
    printf("Multi turn pos: %l \n", *mangle);
    //printf("Multi turn pos: %" PRId64 "\n", *mangle);
    #endif
}
 

void GyemsManager::set_pos2zero(int nodeID){
     
    /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0x19;                                            
     tx_buf.data[1] = 0;                
     tx_buf.data[2] = 0;                  
     tx_buf.data[3] = 0;  
     tx_buf.data[4] = 0;            
     tx_buf.data[5] = 0;               
     tx_buf.data[6] = 0;  
     tx_buf.data[7] = 0;    
     
    channel_name.write(tx_buf);
}


void GyemsManager::write_encoder_offset(int nodeID,uint16_t offset){
     
    /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0x91;                                            
     tx_buf.data[1] = 0;                
     tx_buf.data[2] = 0;                  
     tx_buf.data[3] = 0;  
     tx_buf.data[4] = 0;            
     tx_buf.data[5] = 0;                  
     tx_buf.data[6] = offset&0xff;  
     tx_buf.data[7] = (offset>>8)&0xff;      
     
    channel_name.write(tx_buf);
}

void GyemsManager::read_encoder(int nodeID){
     
    /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0x90;                                            
     tx_buf.data[1] = 0;                
     tx_buf.data[2] = 0;                  
     tx_buf.data[3] = 0;  
     tx_buf.data[4] = 0;            
     tx_buf.data[5] = 0;                  
     tx_buf.data[6] = 0;  
     tx_buf.data[7] = 0;      
     
    channel_name.write(tx_buf);
}

void GyemsManager::unpack_read_encoder(struct can_frame* rx_msg, uint16_t* pos, uint16_t* raw, uint16_t* offset){
    
    *pos = (rx_msg->data[3]<<8)|rx_msg->data[2];
    *raw = (rx_msg->data[5]<<8)|(rx_msg->data[4]);
    *offset = (rx_msg->data[7]<<8)|rx_msg->data[6];
    #ifdef DEBUG_GYEMS
    printf("pos: %u raw: %u offset: %u \n", *pos, *raw, *offset);
    #endif    
}
 

void GyemsManager::write_acceleration2ram(int nodeID,uint16_t accel){
    //set acceleration limit
    
    /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0x34;                                              
     tx_buf.data[1] = 0;                
     tx_buf.data[2] = 0;                  
     tx_buf.data[3] = 0;               
     tx_buf.data[4] = accel&0xff;  
     tx_buf.data[5] = (accel>>8)&0xff;  
     tx_buf.data[6] = (accel>>16)&0xff;            
     tx_buf.data[7] = (accel>>24)&0xff;        
     
    channel_name.write(tx_buf);
}

void GyemsManager::read_acceleration(int nodeID){
    //read acceleration limit

    /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0x33;                                            
     tx_buf.data[1] = 0;                
     tx_buf.data[2] = 0;                  
     tx_buf.data[3] = 0;  
     tx_buf.data[4] = 0;            
     tx_buf.data[5] = 0;                  
     tx_buf.data[6] = 0;  
     tx_buf.data[7] = 0;      
     
    channel_name.write(tx_buf);

}

void GyemsManager::unpack_read_acceleration(struct can_frame* rx_msg, int32_t* paccel){
    
    *paccel = (rx_msg->data[7]<<24)|(rx_msg->data[6]<<16)|(rx_msg->data[5]<<8)|(rx_msg->data[4]);
    
    #ifdef DEBUG_GYEMS
    printf("accel: %ld \n", *paccel);
    #endif    

}

void GyemsManager::write_PID(int nodeID,PIDconstant pid){
     
    /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0x31;                                            
     tx_buf.data[1] = 0;                
     tx_buf.data[2] = pid.anglePidKp&0xff;  
     tx_buf.data[3] = pid.anglePidKi&0xff;  
     tx_buf.data[4] = pid.speedPidKp&0xff;  
     tx_buf.data[5] = pid.speedPidKi&0xff;             
     tx_buf.data[6] = pid.iqPidKp&0xff;  
     tx_buf.data[7] = pid.iqPidKi&0xff;  
     
    channel_name.write(tx_buf);
}


void GyemsManager::read_PID(int nodeID){
     
    /// pack ints into the can buffer ///
     tx_buf.can_id  = 0x140+nodeID;
	 tx_buf.can_dlc = 8;
	 tx_buf.data[0] = 0x30;                                            
     tx_buf.data[1] = 0;                
     tx_buf.data[2] = 0;                  
     tx_buf.data[3] = 0;  
     tx_buf.data[4] = 0;            
     tx_buf.data[5] = 0;                  
     tx_buf.data[6] = 0;  
     tx_buf.data[7] = 0;      
     
    channel_name.write(tx_buf);
    
}

void GyemsManager::unpack_read_PID(struct can_frame* rx_msg, struct PIDconstant* pid){
    
    pid->anglePidKp = rx_msg->data[2];
    pid->anglePidKi = rx_msg->data[3];
    pid->speedPidKp = rx_msg->data[4];
    pid->speedPidKi = rx_msg->data[5];
    pid->iqPidKp = rx_msg->data[6];
    pid->iqPidKi = rx_msg->data[7];
    
    #ifdef DEBUG_GYEMS
    printf("anglePidKp: %hu anglePidKi: %hu \n", pid->anglePidKp, pid->anglePidKi);
    printf("speedPidKp: %hu speedPidKi: %hu \n", pid->speedPidKp, pid->speedPidKi);
    printf("iqPidKp: %hu iqPidKi: %hu \n", pid->iqPidKp, pid->iqPidKi);
    #endif    
}
 
