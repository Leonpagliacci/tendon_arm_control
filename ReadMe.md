# Readme
This is the Gyems driver using linux socket can

# step of use :
1. initialize socket can devices: direct to tendon_arm_control folder, excute "sudo ./can_init.sh", use "ifconfig" to check if CAN0 is avaliable;
2. change the code as you want, do not have to modify main.cpp unless motors are not in CAN0. Change the code in  Controller_test.cpp, i.e pack_position_6_cmd(can node ID, incremental angle in 0.01 degree, max speed in degree/sec) and move your motors to anywhere you want!
3. compile the project: go to test_motors/,excute "mkdir build", go into build/ and excute "cmake .." (all Cmake file is fairly configured), the "make";
4. run the code. in build/ run "sudo ./main". press ctrl+C to stop the motors.

## Folder description:
- modules: is the folder of many libraries can be called by projects
- test**: are folders of projects, may have different versions.

## why this 
This setup allows different version of the same module. 
The projects are independent as well. 

Each project has an independent cmakeList file to set up the libraries needed.

## Project descriptions:



# notes:
- using RT thread requires sudo command to excute program.
