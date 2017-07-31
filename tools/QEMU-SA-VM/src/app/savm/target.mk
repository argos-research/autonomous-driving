proto_cc := $(wildcard $(REP_DIR)/../protobuf/build/*.pb.cc)
proto_h := $(REP_DIR)/../protobuf/build
TARGET = savm
SRC_CC = main.cc $(proto_cc) tcp_socket.cc publisher.cc subscriber.cc
LIBS   = base libmosquitto stdcxx lwip pthread libprotobuf

INC_DIR += $(call select_from_repositories,include/lwip)
INC_DIR += $(proto_h) $(REP_DIR)
