#ifndef SD2_HH
#define SD2_HH

// internal
#include <master_sim.hh>
// boost
#include <boost/asio.hpp>
// protobuf
#include <setup.pb.h>
#include <state.pb.h>

class SD2 : public MASTER_SIM {
public:
	SD2(std::string address, int port);
	~SD2();
	int getNumberOfSimulationObjects();
	void simulationStep() override;

private:
	// boost
	boost::asio::io_service io_service;
	boost::asio::ip::tcp::socket s;

	void receivePacket(std::vector<uint8_t> &buffer);
	void receiveTrack();
	void receiveSituation();
};

#endif
