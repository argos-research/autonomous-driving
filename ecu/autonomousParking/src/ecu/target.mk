TARGET = ecu

SRC_CC += main.cc
SRC_CC += Parking.cc $(REP_DIR)/../../tools/QEMU-SA-VM/src/app/savm/subscriber.cc $(REP_DIR)/../../tools/QEMU-SA-VM/src/app/savm/publisher.cc
          
LIBS   = base libmosquitto stdcxx lwip pthread

INC_DIR += $(call select_from_repositories,include/lwip)
INC_DIR += $(REP_DIR)/../../tools/QEMU-SA-VM/src/app/savm
