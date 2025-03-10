# Readme
this directory is to IMU sensing project.


## Folder description:
- modules: is the folder of many libraries can be called by projects
- test**: are folders of projects, may have different versions.

## why this 
This setup allows different version of the same module. 
The projects are independent as well. 

Each project has an independent cmakeList file to set up the libraries needed.

## Project descriptions:

- test_socket_can: Initial version for testings. 
- test_simpletask: this project is to test rt task, with spdlog.



# notes:
- using RT thread requires sudo command to excute program.
