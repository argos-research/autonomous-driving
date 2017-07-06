/*
 * SpeedDreams2 (SD2)
 * sd2.cc
 *
 * Purpose: Handles the connection to SpeedDreams2 (SD2).
 * Receives data about the track and vehicles in the simulation
 * and provides it for other simulators.
 */

#include <sd2.hh>

// boost
#include <boost/asio.hpp>
// protobuf
#include <setup.pb.h>
#include <state.pb.h>

SD2::SD2(std::string address, int port) : s(io_service) {
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(address), port);
	boost::asio::ip::tcp::acceptor ac(io_service, ep);
	ac.accept(s);
	receiveTrack();
};

SD2::~SD2() {
	// TODO correctly destr...close boost stuff
};

void SD2::simulationStep() {
	receiveSituation();
};

int SD2::getNumberOfSimulationObjects() {
	return getCurrentState().vehicles_size();
};

/*
 * Receive a Packet from SD2 (4 byte message length + message)
 */
void SD2::receivePacket(std::vector<uint8_t> &buffer) {
	uint32_t length = 0, bread = 0;

	// get message length
	boost::asio::read(s, boost::asio::buffer(&length, 4));
	length = ntohl(length);

	// receive data
	buffer.resize(length);
	bread = boost::asio::read(s, boost::asio::buffer(buffer, length));
	if (bread != length); // TODO error receiveing packet
};

/*
 * Receive and parse track message
 */
void SD2::receiveTrack() {
	std::vector<uint8_t> message;
	receivePacket(message);

	if (getSetup().ParseFromArray(message.data(), message.size()));
	else ; // TODO error parsing packet
};

/*
 * Receive and parse situation message
 */
void SD2::receiveSituation() {
	std::vector<uint8_t> message;
	receivePacket(message);

	if (getCurrentState().ParseFromArray(message.data(), message.size()));
	else ; // TODO error parsing
};
