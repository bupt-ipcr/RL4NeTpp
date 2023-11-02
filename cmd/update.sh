#!/bin/bash
###
 # @Author       : CHEN Jiawei
 # @Date         : 2023-10-18 20:28:25
 # @LastEditors  : LIN Guocheng
 # @LastEditTime : 2023-10-19 17:05:04
 # @FilePath     : /home/lgc/test/RL4Net++/cmd/update.sh
 # @Description  : Update edited modules to the path of omnetpp and inet, including file copying and compilation.
### 

# replace the following paths with your omnetpp and inet path
omnetpp_path="../omnetpp-5.6.1"
inet_path="../inet4"

current_path=$(pwd)

# copy edited module files
cp -r ./modules/omnetpp/* $omnetpp_path/src/sim/
cp -r ./modules/inet/ipv4/* $inet_path/src/inet/networklayer/ipv4/
cp -r ./modules/inet/udpapp/* $inet_path/src/inet/applications/udpapp/

# compile omnetpp and inet
cd $omnetpp_path
make -j32
. setenv
cd $current_path

cd $inet_path
make -j32
. setenv
cd $current_path
