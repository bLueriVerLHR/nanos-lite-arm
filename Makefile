PREFIX := arm-none-eabi-

CC := $(PREFIX)gcc
OC := $(PREFIX)objcopy
OD := $(PREFIX)objdump


BIN 			:= armos
BUILD 			:= build
BIN_BUILD 		:= $(BUILD)/$(BIN)
ABS_BUILD_DIR 	:= $(CURDIR)/$(BUILD)

CM0_CSRC 		:= boot.c start.c
CM0_LDSRC 		:= cm0.ld
CM0_OBJ 		:= $(patsubst %.c,%.o,$(CM0_CSRC))
CM0_OBJ_BUILD 	:= $(addprefix $(BUILD)/,$(CM0_OBJ))

# C sources are compiled in cortex-m0 env
COMMONFLAGS := -mtune=cortex-m0 # -march=armv6s-m 
CPPFLAGS 	:= -DDEBUG_MODE=1
CFLAGS 		:= $(COMMONFLAGS) -nostartfiles
LDFLAGS 	:= $(COMMONFLAGS) $(addprefix -T,$(CM0_LDSRC)) -nostartfiles # -nostdlib

# C++ sources are compiled in native env
CXXFLAGS	+= -std=c++2a

USIM 			:= usim
USIM_BUILD_DIR	:= $(ABS_BUILD_DIR)/$(USIM)
USIM_SRC 		:= $(shell find $(USIM) -name '*.cpp')
USIM_SRC_NODIR 	:= $(notdir $(USIM_SRC))
USIM_OBJ 		:= $(patsubst %.cpp,%.o,$(USIM_SRC_NODIR))
USIM_OBJ_BUILD 	:= $(addprefix $(USIM_BUILD_DIR)/,$(USIM_OBJ))
USIM_BUILD 		:= $(BUILD)/$(USIM).cm0

SYSCALL 			:= syscall
SYSCALL_BUILD_DIR	:= $(ABS_BUILD_DIR)/$(SYSCALL)
SYSCALL_SRC 		:= $(shell find $(SYSCALL) -name '*.c')
SYSCALL_SRC_NODIR 	:= $(notdir $(SYSCALL_SRC))
SYSCALL_OBJ 		:= $(patsubst %.c,%.o,$(SYSCALL_SRC_NODIR))
SYSCALL_OBJ_BUILD 	:= $(addprefix $(SYSCALL_BUILD_DIR)/,$(SYSCALL_OBJ))

NANOS			:= nanos-lite
NANOS_BUILD_DIR	:= $(ABS_BUILD_DIR)/$(NANOS)
NANOS_SRC 		:= $(shell find $(NANOS) -name '*.c')
NANOS_SRC_NODIR := $(notdir $(NANOS_SRC))
NANOS_OBJ 		:= $(patsubst %.c,%.o,$(NANOS_SRC_NODIR))
NANOS_OBJ_BUILD := $(addprefix $(NANOS_BUILD_DIR)/,$(NANOS_OBJ))

AM				:= am
AM_BUILD_DIR	:= $(ABS_BUILD_DIR)/$(AM)
AM_SRC 			:= $(shell find $(AM) -name '*.c')
AM_SRC_NODIR 	:= $(notdir $(AM_SRC))
AM_OBJ 			:= $(patsubst %.c,%.o,$(AM_SRC_NODIR))
AM_OBJ_BUILD 	:= $(addprefix $(AM_BUILD_DIR)/,$(AM_OBJ))

ALL_BUILD_DIR 	:= $(USIM_BUILD_DIR) $(SYSCALL_BUILD_DIR) $(NANOS_BUILD_DIR) $(AM_BUILD_DIR)
ALL_OBJ 		:= $(CM0_OBJ_BUILD) $(SYSCALL_OBJ_BUILD) $(NANOS_OBJ_BUILD) $(AM_OBJ_BUILD)

$(BUILD):
	mkdir -p $(BUILD)

$(USIM_BUILD_DIR): $(BUILD)
	mkdir -p $(USIM_BUILD_DIR)

$(SYSCALL_BUILD_DIR): $(BUILD)
	mkdir -p $(SYSCALL_BUILD_DIR)
	
$(NANOS_BUILD_DIR): $(BUILD)
	mkdir -p $(NANOS_BUILD_DIR)

$(AM_BUILD_DIR): $(BUILD)
	mkdir -p $(AM_BUILD_DIR)

$(CM0_OBJ_BUILD): $(BUILD)/%.o:%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $^ -o $@

$(USIM_OBJ_BUILD): $(USIM_BUILD_DIR)/%.o:$(USIM)/%.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

$(USIM_BUILD): $(USIM_OBJ_BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(SYSCALL_OBJ_BUILD): $(SYSCALL_BUILD_DIR)/%.o:$(SYSCALL)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $^ -o $@

$(NANOS_OBJ_BUILD): $(NANOS_BUILD_DIR)/%.o:$(NANOS)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $^ -o $@

$(AM_OBJ_BUILD): $(AM_BUILD_DIR)/%.o:$(AM)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $^ -o $@

all: usim arm

arm: $(ALL_BUILD_DIR) $(ALL_OBJ)
	$(CC) -o $(BIN_BUILD) $(ALL_OBJ) $(LDFLAGS)
	$(OD) -D $(BIN_BUILD) > $(BIN_BUILD).dump
	$(OC) $(BIN_BUILD) -O binary $(BIN_BUILD).bin

usim: $(USIM_BUILD_DIR) $(USIM_BUILD)

utest: $(USIM_BUILD) $(BIN_BUILD).bin
	$(USIM_BUILD) $(BIN_BUILD).bin

clean:
	rm -r $(BUILD)