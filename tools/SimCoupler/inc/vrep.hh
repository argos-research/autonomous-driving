#ifndef VREP_HH
#define VREP_HH

// internal
#include <master_sim.hh>

class VREP : public MASTER_SIM {
public:
  VREP(std::string address, int port);
  ~VREP();
  void simulationStep();

private:
  int clientID = 0;
};

#endif
