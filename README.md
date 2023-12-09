<h2 align="center">RL4NeT++: A Packet-Level Network Simulation Framework for DRL-Based Routing Algorithms</h2>
<p align="center"><b>Intelligent Sensing and Computing Research Center / School of Artificial Intelligence / Beijing University of Posts and Telecommunications</b></p>



## Table of Contents

- [Table of Contents](#table-of-contents)
- [Directory Structure](#directory-structure)
- [Environment Setup](#environment-setup)
	- [Conda Environment Setup](#conda-environment-setup)
	- [OMNeT++ Installation](#omnet-installation)
	- [ZMQ Installation](#zmq-installation)
	- [INET Installation](#inet-installation)
- [Usage Instructions](#usage-instructions)
	- [Modify Simulation Mode](#modify-simulation-mode)
	- [Run Simulation Using Python](#run-simulation-using-python)
	- [Interact with OMNeT++ Process](#interact-with-omnet-process)
- [Example Python File](#example-python-file)
- [Contributors](#contributors)
- [Contact](#contact)

## Directory Structure

```
├── README.md
├── environment.yml  		// conda environment setup file
├── cmd 			// scripts for controlling simulation environment execution and simulation processes
	├── kill.sh 		// Script for terminating running simulation processes
	├── start.sh		// Script for starting a simulation process
	└── update.sh 		// Script for updating the simulation environment and restart it
├── config 			// Configuration files for the simulation environment
	├── omnetpp.ini 	// OMNeT++ initialization file
	└── ned 		// Network topologies and initial probability tables
├── modules 			// Components of the simulation environment
	├── init.py
	├── gym_env 		// Communication module connecting Python and OMNeT++
	├── omnetpp 		// Addon files to OMNeT++
	└── inet 		// Addon files to INET
└── utils 			// Tools for users' convenience
	├── get_ned.py 		// Convert topology files from GML files to NED file
	├── get_ospf_ned.py 	// Replace routers in the topology information with OSPF
	└── get_rip_ned.py 	// Replace routers in the topology information with RIP
```

## Environment Setup

In the current directory, download the RL4Net++ source code:

```bash
git clone https://github.com/bupt-ipcr/RL4NeTpp.git
```

### Conda Environment Setup

In RL4NeTpp folder, install the `conda` environment named "pfrp" using the `environment.yml` file:

```
cd RL4Netpp
conda env create -f environment.yml
```

### OMNeT++ Installation

Afterward, download the omnetpp 5.6.1 installation package from the official website:

```shell
cd ../
wget https://github.com/omnetpp/omnetpp/releases/download/omnetpp-5.6.1/omnetpp-5.6.1-src-linux.tgz
```

Extract it to the current directory:

```shell
tar zxvf omnetpp-5.6.1-src-linux.tgz -C ./
```

Navigate to the "omnetpp-5.6.1" folder and make the following changes in the `configure.user` file:

```shell
WITH_TKENV=no     # Used to enable a graphical interface based on Tcl/Tk (not needed here)
WITH_QTENV=no     # Used to enable a graphical interface based on Qt
WITH_OSG=no       # Used to enable OpenSceneGraph in the Qt-based graphical interface
WITH_OSGEARTH=no  # Same as above, used to enable osgEarth (requires enabling the previous option)
```

Then, replace `RL4Net++/modules/omnetpp/cdataratechannel.cc` with `omnetpp-5.6.1/src/sim/cdataratechannel.cc` in the current directory and run the following commands to set up the environment variables:

```shell
cp -f RL4Netpp/modules/omnetpp/cdataratechannel.cc omnetpp-5.6.1/src/sim/
cd omnetpp-5.6.1/
. setenv
```

Next, install the necessary OSG libraries:

```shell
apt install software-properties-common
add-apt-repository ppa:ubuntugis/ppa
apt update
apt install openscenegraph-plugin-osgearth libosgearth-dev
```

Install other dependencies like bison, build-essential, gcc, g++…

```shell
apt install build-essential gcc g++ bison flex perl tcl-dev tk-dev blt libxml2-dev zlib1g-dev doxygen graphviz openmpi-bin libopenmpi-dev libpcap-dev default-jre
```

Configure and build OMNeT++:

```shell
./configure
make
```

After compilation, test if OMNeT++ is working properly:

```shell
cd samples/dyna/
./dyna
```

If it runs successfully, the installation is complete.

### ZMQ Installation

Return to the current directory and prepare to install the sodium library, a dependency of zmq:

```shell
cd ../../../
git clone https://github.com/jedisct1/libsodium.git
cd libsodium/
```

Configure, compile, and install the library:

```shell
apt install autoconf
./autogen.sh -s
./configure
make -j32
sudo make install
```

Then, write the compiled dynamic library files to `/usr/local/include/`:

```shell
ldconfig
```

Install ZMQ:

```shell
apt install libzmq3-dev
```

### INET Installation

Return to the current directory and install INET version 4.2.1:

```shell
cd ../
wget https://github.com/inet-framework/inet/releases/download/v4.2.1/inet-4.2.1-src.tgz
tar zxvf inet-4.2.1-src.tgz -C ./
```

After extraction, replace modified files in INET:

```shell
cp -f RL4Netpp/modules/inet/Makefile inet4/
cp -r -f ./modules/inet/ipv4/* inet4/src/inet/networklayer/ipv4/
cp -r -f ./modules/inet/udpapp/* inet4/src/inet/applications/udpapp/
```

Compile INET:

```shell
cd inet4/
make makefiles
make -j32
. setenv
```

The installation is now complete.

## Usage Instructions

### Modify Simulation Mode

- **Traditional Routing Algorithms**:  Change `simMode`  in `config/omnetpp.ini` file to 0.
- **Single-Agent Reinforcement Learning Algorithm(SADRL)**:  Change `simMode` to 1.
- **Multi-Agent Reinforcement Learning Algorithm(MADRL)**:  Change `simMode` to 2.

In `config/omnetpp.ini` , update the network topology information that needs to be simulated in `network` and `routingFileName` parameters. Other parameters can be adjusted according to simulation requirements.

### Run Simulation Using Python

Next, write Python files for running and interacting with OMNeT++. At the beginning of the Python code file, import the `OmnetEnv` class:

```python
from modules.gym_env.omnet_env import OmnetEnv
```

Before the main code, initialize `OmnetEnv` based on Gym:

```python
env = OmnetEnv()
env = env.unwrapped
```

If you are using SADRL, add the following statement:

```python
env.is_multi_agent = False
```

For MADRL, add:

```python
env.is_multi_agent = True
```

Before each episode training starts, call `env.reset()` to start and reset the network simulation environment.

### Interact with OMNeT++ Process

Use the `env.get_obs()` function to obtain three parameters sent back from OMNeT++:

- **The first parameter:** String type, with values "s" or "r", representing whether the information returned is state or reward.
- **The second parameter:** Integer type, representing the step number.
- **The third parameter:** String type, containing the message body.
  - If the first parameter is "s" the third parameter contains link load information during the network simulation.
  - If the first parameter is "r" the third parameter contains delay and packet loss rate information for the current step.

If `env.get_obs()` returns state information, input it into the DRL algorithm, and the agent will output a set of action values. Convert these action values to a string and use `env.make_action(action_str)` to send the action string to OMNeT++.

If `env.get_obs()` returns reward information, immediately call `env.reward_rcvd()` after obtaining the information to inform OMNeT++ that the reward has been received.

After completing an episode of training in the reinforcement learning algorithm, use `env.close()` to close the simulation environment.

## Example Python File

Here's an example for a simple SADRL algorithm:

```python
from modules.gym_env.omnet_env import OmnetEnv
import ...

env = OmnetEnv()
env = env.unwrapped
env.is_multi_agent = False

agent = ...

while current_episode < MAX_EPISODE:
	env.reset()
  	...
  	while current_step < MAX_STEP_PER_EPISODE:
      	s_or_r, step, msg = env.get_obs()
      	if s_or_r == "s":
        	...
        	state = torch.FloatTensor(msg)
          	action = agent.make_action(state)
          	...
          	action_str = "".join(
            	            f"{str(torch.sigmoid(a).tolist()[0][j])},"
              	          		for j in range(action_dim)
                	    	)
	        action_str = action_str[:-1]
  	        env.make_action(action_str)
    	elif s_or_r == "r":
      	    delay = float(msg.split(",")[0])
        	loss_rate = float(msg.split(",")[1])
          	...
	        env.reward_rcvd()
	if env:
      	env.close()
```

## Contributors

- [Jiawei Chen](https://github.com/Chen-J-W) 
- [Yang Xiao](https://github.com/gnayoaix)
- [Guocheng Lin](https://github.com/lgc0208)
- [Jun Liu](https://github.com/liujunlj)

## Contact

Guocheng Lin ([linguocheng@bupt.edu.cn](mailto:linguocheng@bupt.edu.cn)), Beijing University of Posts and Telecommunications, China
