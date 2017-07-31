/*
 * S/A VM
 * savm.cc
 *
 * Purpose: Handles the connection to a S/A VM.
 * Transmits the current state of the simulation
 * and receives control information for the next step.
 */

#include <savm.hh>

// boost
#include <boost/asio.hpp>

SAVM::SAVM(std::string address, int port) : s(io_service) {
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(address), port);
	boost::asio::ip::tcp::acceptor ac(io_service, ep);
	ac.accept(s);
};

SAVM::~SAVM() {
	// TODO correctly destr...close boost stuff
};
