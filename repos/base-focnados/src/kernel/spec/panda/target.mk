REQUIRES       = panda
FIASCO_DIR    := $(call select_from_ports,focnados)/src/kernel/foc/kernel/fiasco
KERNEL_CONFIG  = $(REP_DIR)/config/panda.kernel

-include $(PRG_DIR)/../../target.inc
