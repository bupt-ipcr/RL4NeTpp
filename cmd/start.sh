#!/bin/bash
###
 # @Author       : CHEN Jiawei
 # @Date         : 2023-10-18 20:28:25
 # @LastEditors  : LIN Guocheng
 # @LastEditTime : 2023-10-18 21:03:59
 # @FilePath     : /home/lgc/test/RL4Net++/cmd/start.sh
 # @Description  : Start omnetpp, inet and conda environment before simulation.
### 

# replace the following paths with your omnetpp and inet path
omnetpp_path="../omnetpp-5.6.1"
inet_path="../inet4.5"

current_path=$(pwd)

# replace with your conda environment name
conda_name="pfrp"

# start omnetpp and inet environment
cd $omnetpp_path
. setenv
cd $current_path

cd $inet_path
. setenv
cd $current_path

# start conda environment
conda activate $conda_name