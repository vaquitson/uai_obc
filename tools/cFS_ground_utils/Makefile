CFS_ROOT := ../..
BUILD_DIR := $(CFS_ROOT)/build
ARCH := native/default_cpu1
ARCH_BUILD_DIR := $(BUILD_DIR)/$(ARCH)

CC := gcc
CFLAGS := -g -Wall -Wextra -std=c99 \
	-DSIMULATION=native -D_LINUX_OS_ -D_POSIX_OS_ -D_XOPEN_SOURCE=600

INCLUDES := \
	-I$(BUILD_DIR)/inc \
	-I$(ARCH_BUILD_DIR)/inc \
	-I$(CFS_ROOT)/cfe/modules/core_api/fsw/inc \
	-I$(CFS_ROOT)/cfe/modules/es/fsw/inc \
	-I$(CFS_ROOT)/cfe/modules/evs/fsw/inc \
	-I$(CFS_ROOT)/cfe/modules/fs/fsw/inc \
	-I$(CFS_ROOT)/cfe/modules/sb/fsw/inc \
	-I$(CFS_ROOT)/cfe/modules/tbl/fsw/inc \
	-I$(CFS_ROOT)/cfe/modules/time/fsw/inc \
	-I$(CFS_ROOT)/cfe/modules/msg/fsw/inc \
	-I$(CFS_ROOT)/cfe/modules/msg/fsw/src \
	-I$(CFS_ROOT)/cfe/modules/resourceid/fsw/inc \
	-I$(CFS_ROOT)/cfe/modules/config/fsw/inc \
	-I$(CFS_ROOT)/osal/src/os/inc \
	-I$(ARCH_BUILD_DIR)/osal/inc \
	-I$(CFS_ROOT)/psp/fsw/inc \
	-I$(ARCH_BUILD_DIR)/psp/inc

MSG_LIB := $(ARCH_BUILD_DIR)/msg/libmsg.a

TARGET := send_cfs_cmd
SRCS := send_cfs_cmd.c cfe_msg_ground_stubs.c

all: $(TARGET)

$(TARGET): $(SRCS) $(MSG_LIB)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SRCS) $(MSG_LIB)

clean:
	rm -f $(TARGET)
