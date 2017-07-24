TARGET = receiver
SRC_CC = main.cpp
LIBS   = base libmosquitto stdcxx lwip pthread

INC_DIR += $(call select_from_repositories,include/lwip)
