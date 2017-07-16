// simulators
#include <sd2.hh>
#include <vrep.hh>
#include <sumo.hh>
// protobuf
#include <setup.pb.h>

#define SIM_SD2

int main(int argc, char *argv[]) {
  MASTER_SIM* ms;

  /* create module for master simulation */
  #ifdef SIM_SD2
  ms = new SD2("127.0.0.1", 9000);
  #endif

  #ifdef SIM_VREP
  ms = new VREP("127.0.0.1", 9000);
  #endif

  /* create modules for secondary simulators */
  #ifdef SIM_SD2
  // get setup from main simulation
  protobuf::Setup track = ms->getSetup();
  SUMO sumo("127.0.0.1", 2002, track);
  #endif

  /* TODO create modules for S/A VMs */

  /* main loop
   *
   * 1. execute simulation step in master simulation
   * 2. get current state from master simulation
   * 3. pass current state to secondary simulators
   * 4. pass current state to S/A VMs
   */
  protobuf::State currentState;
  while(true) {
    ms->simulationStep();
    currentState = ms->getCurrentState();

    #ifdef SIM_SD2
    sumo.simulationStep(currentState);
    #endif

    // TODO S/A VMs
  }
}
