SPECS += focnados_arm rpi

include $(call select_from_repositories,mk/spec/rpi.mk)
include $(call select_from_repositories,mk/spec/focnados_arm.mk)
