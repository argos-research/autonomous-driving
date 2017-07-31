---
title: Team speedDreams (Group 03)
author:
- Alexander Reisner
- Alexander Weidinger
- David Werner
institute: Technical University of Munich (TUM)
abstract: This repository contains the source code of 'Team speedDreams', which implements an autonomous parking scenario for the lil4 practical course.
---

# Repository structure
```
.
+-- ecu                    // (contains code for the ecus)
|   +-- autonomousParking  // (our autonomous parking ecu)
+-- genode                 // (the official genode 16.08)
+-- simulators             // (the collection of simulators in use)
|   +-- speed-dreams       // (also contains the code changes done to speed-dreams)
|   +-- sumo               // (was planned to be used in conjunction with the SimCoupler but was omitted for this practical course)
+-- tools                  // (additional tools or utility software)
|   +-- genode-world       // (genode-world repository of argos-research)
|   +-- protobuf           // (contains the protobuf message definitions of *State* and *Control*)
|   +-- QEMU-SA-VM         // (contains code for the S/A VM)
|   +-- SimCoupler         // (was planned to be used as module between SD2 and S/A VM but was omitted for this pracitcal course)
+-- Makefile               // (used in SA/VM)
+-- README.md              // (Readme of this project)
```

# Dependencies
- [Mosquitto](https://mosquitto.org/) (https://mosquitto.org/), which is not provided by this repository
- Run time dependencies, which can be looked up in the respective directories of the simulators and genode

# Build instructions
- The repository consists of multiple submodules, which first need to be fetched via `git submodule update --init`.

## Speed-dreams 2
In simulators/speed-dreams execute the following commands:
```
mkdir -p build
cd build
cmake ../ -DCMAKE_CXX_FLAGS=-fpermissive -DOPTION_PARKING=ON
make
```

## SA/VM
```
cd genode/repos
ln -s ../../tools/genode-world world    # link the genode-world repository into the genode file structure
cd ../../
make jenkins_build_dir                  # creates a build directory, use the Makefile to configure different platforms, etc.
make toolchain                          # prepares the 16.08 toolchain of genode, used for the compilation process
make ports                              # prepares necessary library ports for genode
make vde                                # starts up a vde_switch and tap device

# Please make sure you have isohybrid installed, which is part of the syslinux-utils (Ubuntu)
```

## ECU
- Please follow the instructions in SA/VM, but it's only necessary to do this process once (so don't redo it if you already did all the step in SA/VM).

# Configuration
- One can adapt the address and port SD2 listens on for the QEMU SA/VM connection in simulations/speed-dreams/src/drivers/human/human.cpp
- For the SA/VM and ECU, one can adapt the configuration by using the `*.run` files in the respective directories

# Run instructions
- Start a mosquitto server
- Start speed-dreams 2 by executing `./simulators/speed-dreams/build/games/speed-dreams-2`
- Configure a race in speed-dreams to consist of **3 usr** bots and **1 human** player (use the espie track) and start it
- Start up the QEMU SA/VM by executing `make -C genode/build/foc_x86_64/ run/savm` (it will connect to speed-dreams)
- Start up the QEMU ECU by executing `make -C genode/build/foc_x86_64/ run/ecu`

# Execute parking scenario
- In speed-dreams: press the *'enter'* key and reduce the simulation time to 0.25 by pressing the *'-'* key two times (this may be unnecessary, depending on your configuration - if you're unsure, just use 0.25 and slowly increase the simulation speed with the *'+'* key until it becomes unresponsive)
- Use the provided mosquitto_pub by mosquitto to publish the go command: `./mosquitto_pub -h <ip-address> -p <port> -t state -m 'go; 1'`
- The bot will now autonomously park, the controls will be reactivated once the car is done parking
- To abort the parking manoeuver, just publish go with value 0: `./mosquitto_pub -h <ip-address> -p <port> -t state -m 'go; 0'`
