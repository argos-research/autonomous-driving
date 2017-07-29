---
title: Team speedDreams
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
+-- genode
|   +-- ...
+-- simulators             // (the collection of simulators in use)
|   +-- speed-dreams
|   +-- sumo
+-- tools                  // (additional tools or utility software)
|   +-- QEMU-SA-VM
|   +-- SimCoupler
```

# Build instructions
- The repository consists of multiple submodules, which first need to be fetched via `git submodule update --init`.

## Speed-dreams 2
In simulators/speed-dreams execute the following commands:
```
mkdir -p build
cd build
cmake ../ -DCMAKE_CXX_FLAGS=-fpermissive -DOPTION_PARKING=ON
make```

## SA/VM
*prepare ports*
*link directory*
...

## ECU
*prepare ports*
*link directory*
...

# Run instructions
- Start a mosquitto server
- Start speed-dreams 2 by executing `./simulators/speed-dreams/build/games/speed-dreams-2`
- Configure a race in speed-dreams to consist of **3 usr** bots and **1 human** player (use the espie track) and start it
- Start up the QEMU SA/VM by executing `make -C genode/build/foc_x86_64/ run/savm` (it will connect to speed-dreams)
- Start up the QEMU ECU by executing `make -C genode/build/foc_x86_64/ run/ecu`

# Execute parking scenario
- In speed-dreams: press the *'enter'* key and reduce the simulation time to 0.25 by pressing *'-'* two times
- Use the provided mosquitto_pub by mosquitto to publish the go command: `./mosquitto_pub -h <ip-address> -p <port> -t state -m 'go; 1'`
- The bot will now autonomously park, the controls will be reactivated once the car is done parking
- To abort the parking manoeuver, just publish go with value 0: `./mosquitto_pub -h <ip-address> -p <port> -t state -m 'go; 0'`
