#ifndef MASTER_SIM_HH
#define MASTER_SIM_HH

// protobuf
#include <state.pb.h>
#include <setup.pb.h>

class MASTER_SIM {
public:
  protobuf::Setup& getSetup() { return setup; };
  protobuf::State& getCurrentState() { return currentState; };

  virtual void simulationStep() = 0;

private:
  protobuf::Setup setup;
  protobuf::State currentState;
};

#endif
