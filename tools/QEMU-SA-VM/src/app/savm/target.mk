proto_cc := $(wildcard $(REP_DIR)/../protobuf/build/*.c)
proto_h := $(REP_DIR)/../protobuf/build
TARGET = savm
SRC_CC = main.cpp $(proto_cc) tcp_socket.cc
LIBS   = base libmosquitto stdcxx lwip pthread libprotobuf

INC_DIR += $(call select_from_repositories,include/lwip)
INC_DIR += $(proto_h) $(REP_DIR)
