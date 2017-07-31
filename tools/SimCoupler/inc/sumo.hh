#ifndef SUMO_HH
#define SUMO_HH

// XXX include order matters for protobuf and traci
// protobuf
#include <setup.pb.h>
#include <state.pb.h>
// traci
#include <utils/traci/TraCIAPI.h>

class SUMO : public TraCIAPI {
public:
  SUMO(std::string address, int port, protobuf::Setup &track);
  ~SUMO();
  void simulationStep(protobuf::State &situation);

private:
  float adjustX = 0, adjustY = 0;

  protobuf::Setup track;
  float convertYawToAngle(float yaw);
  void moveVehicleToXY(std::string name, float x, float y, float angle);
};

#endif
