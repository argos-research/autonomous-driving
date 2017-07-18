#proto_cc := $(REP_DIR)/../protobuf/build/sensor.pb.cc
proto_h := $(REP_DIR)/../protobuf/build/state.pb.h
TARGET = savm
SRC_CC = main.cpp tcp_socket.h $(proto_h) #$(proto_cc)
LIBS   = base libmosquitto stdcxx lwip pthread

INC_DIR += $(call select_from_repositories,include/lwip)
