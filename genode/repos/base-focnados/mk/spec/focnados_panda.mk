SPECS += focnados_arm panda

include $(call select_from_repositories,mk/spec/panda.mk)
include $(call select_from_repositories,mk/spec/focnados_arm.mk)
