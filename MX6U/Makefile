APP_NAME := mx6u_skeleton
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build

CROSS_COMPILE ?=
SYSROOT ?=

CC := $(if $(filter default undefined,$(origin CC)),$(CROSS_COMPILE)gcc,$(CC))
CXX := $(if $(filter default undefined,$(origin CXX)),$(CROSS_COMPILE)g++,$(CXX))
MX6U_ENABLE_OPENCV ?= $(if $(filter Windows_NT,$(OS)),0,1)
CFLAGS := -std=c11 -Wall -Wextra -I$(INC_DIR)
CXXFLAGS := -std=c++11 -Wall -Wextra -I$(INC_DIR) -DMX6U_ENABLE_OPENCV=$(MX6U_ENABLE_OPENCV)
LDFLAGS :=
LDLIBS :=

ifneq ($(strip $(SYSROOT)),)
CFLAGS += --sysroot=$(SYSROOT)
CXXFLAGS += --sysroot=$(SYSROOT)
LDFLAGS += --sysroot=$(SYSROOT)
endif

ifeq ($(MX6U_ENABLE_OPENCV),1)
LDLIBS += -lopencv_videoio -lopencv_imgproc -lopencv_core -lopencv_highgui
endif

ifeq ($(OS),Windows_NT)
TARGET_EXT := .exe
MKDIR_BUILD = if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
RM_BUILD = if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"
RUN_TARGET = $(TARGET)
LDLIBS += -lws2_32
else
TARGET_EXT :=
MKDIR_BUILD = mkdir -p $(BUILD_DIR)
RM_BUILD = rm -rf $(BUILD_DIR)
RUN_TARGET = ./$(TARGET)
endif

CSRCS := $(wildcard $(SRC_DIR)/*.c)
CPPSRCS := $(wildcard $(SRC_DIR)/*.cpp)
COBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(CSRCS))
CPPOBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPPSRCS))
OBJS := $(COBJS) $(CPPOBJS)
TARGET := $(BUILD_DIR)/$(APP_NAME)$(TARGET_EXT)
LINKER := $(if $(strip $(CPPSRCS)),$(CXX),$(CC))

.PHONY: all clean help run

all: $(TARGET)

$(BUILD_DIR):
	$(MKDIR_BUILD)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(LINKER) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

clean:
	$(RM_BUILD)

run: $(TARGET)
	$(RUN_TARGET)

help:
	@echo "Targets:"
	@echo "  all    - build $(APP_NAME)"
	@echo "  clean  - remove build artifacts"
	@echo "  run    - run the host-built binary"
	@echo ""
	@echo "Variables:"
	@echo "  CROSS_COMPILE=<toolchain-prefix>"
	@echo "  SYSROOT=<toolchain-sysroot>"
	@echo "  MX6U_ENABLE_OPENCV=0|1"
