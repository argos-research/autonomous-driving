TARGET = savm
SRC_CC = main.cpp tcp_socket.h
LIBS   = base libmosquitto stdcxx lwip pthread

INC_DIR += $(call select_from_repositories,include/lwip)
