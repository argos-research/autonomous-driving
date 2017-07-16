#project
PROJECT ?= hello
PROJECT_DIR ?= hello_tutorial

# options: x86 arm
TOOLCHAIN_TARGET    ?= arm

# options: see tool/create_builddir
GENODE_TARGET       ?= focnados_pbxa9

BUILD_DIR           ?= /build
TOOLCHAIN_BUILD_DIR ?= $(BUILD_DIR)/toolchain-$(TOOLCHAIN_TARGET)
GENODE_BUILD_DIR    ?= $(BUILD_DIR)/genode-$(GENODE_TARGET)
BUILD_CONF           = $(GENODE_BUILD_DIR)/etc/build.conf

all: ports platform

# ================================================================
# Genode toolchain. Only needs to be done once per target (x86/arm).
toolchain:
	mkdir -p $(TOOLCHAIN_BUILD_DIR)
	cd $(TOOLCHAIN_BUILD_DIR);\
	../../tool/tool_chain $(TOOLCHAIN_TARGET)
#
# ================================================================


# ================================================================
# Download Genode external sources. Only needs to be done once per system.
ports: foc libports

foc:
	$(MAKE) -C repos/base-focnados prepare

libports:
	$(MAKE) -C repos/libports prepare
#
# ================================================================


# ================================================================
# Genode build process. Rebuild subtargets as needed.
platform: genode_build_dir

genode_build_dir:
	tool/create_builddir $(GENODE_TARGET) BUILD_DIR=$(GENODE_BUILD_DIR)
	printf 'REPOSITORIES += $$(GENODE_DIR)/repos/libports\n' >> $(BUILD_CONF)
	printf 'REPOSITORIES += $$(GENODE_DIR)/repos/$(PROJECT_DIR)\n' >> $(BUILD_CONF) #add additional repos

# Delete build directory for all target systems. In some cases, subfolders in the contrib directory might be corrupted. Remove manually and re-prepare if necessary.
clean:
	rm -rf $(BUILD_DIR)
#
# ================================================================


# ================================================================
# Run Genode with an active dom0 server.
run:
	$(MAKE) -C $(GENODE_BUILD_DIR) run/$(PROJECT) #declare which run file to run
#
# ================================================================

# ================================================================
# Requiered packages for relaunched systems
packages:
	sudo apt-get update
	sudo apt-get install libncurses5-dev texinfo autogen autoconf2.64 g++ libexpat1-dev flex bison gperf cmake libxml2-dev libtool zlib1g-dev libglib2.0-dev make pkg-config gawk subversion expect git libxml2-utils syslinux xsltproc yasm iasl lynx unzip qemu
#
# ================================================================
