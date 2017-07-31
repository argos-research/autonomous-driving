/*
 * V-REP
 * vrep.cc
 *
 * Purpose: Handles the connection to V-REP.
 * Receives data about the objects and setup of the simulation
 * and provides it for other simulators.
 */

#include <vrep.hh>

//
#include <iostream>

// V-REP
extern "C" {
  #include "extApi.h"
}

VREP::VREP(std::string address, int port) {
  clientID = simxStart(address.c_str(), port, true, true, 5000, 5);

  if (clientID == -1)
    std::cout << "V-REP: Connection failed!" << std::endl;
}

VREP::~VREP() {
  // TODO correctly destr...close stuff
}

void VREP::simulationStep() {
  if (clientID != -1) {
    // TODO
  } else {
    // TODO connection is dead
  }
}
