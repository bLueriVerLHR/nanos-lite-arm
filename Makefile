PREFIX := arm-none-eabi-

CC := $(PREFIX)gcc
OC := $(PREFIX)objcopy
OD := $(PREFIX)objdump

BIN 			:= armos
BUILD 			:= build
BIN_BUILD 		:= $(BUILD)/$(BIN)
ABS_BUILD_DIR 	:= $(CURDIR)/$(BUILD)

LDDIR		:= ldscripts
LDFILES		:= $(shell find $(LDDIR) -name '*.ld')

# C sources are compiled in cortex-m0 env
COMMONFLAGS := -mcpu=cortex-m0 -mlittle-endian
CPPFLAGS 	:= -DDEBUG_MODE -D__OPTIMIZE_SIZE__ -DPREFER_SIZE_OVER_SPEED
CFLAGS 		:= $(COMMONFLAGS) -nostartfiles -mthumb -Os
LDFLAGS 	:= $(COMMONFLAGS) $(addprefix -T,$(LDFILES)) -nostartfiles # -nostdlib

# C++ sources are compiled in native env
CXXFLAGS	+= -std=c++2a


BOOT			:= boot
BOOT_BUILD_DIR	:= $(ABS_BUILD_DIR)/$(BOOT)
BOOT_SRC 		:= $(shell find $(BOOT) -name '*.c')
BOOT_SRC_NODIR 	:= $(notdir $(BOOT_SRC))
BOOT_OBJ 		:= $(patsubst %.c,%.o,$(BOOT_SRC_NODIR))
BOOT_OBJ_BUILD 	:= $(addprefix $(BOOT_BUILD_DIR)/,$(BOOT_OBJ))

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

ALL_BUILD_DIR 	:= $(USIM_BUILD_DIR) $(BOOT_BUILD_DIR) $(SYSCALL_BUILD_DIR) $(NANOS_BUILD_DIR) $(AM_BUILD_DIR)
ALL_OBJ 		:= $(BOOT_OBJ_BUILD) $(SYSCALL_OBJ_BUILD) $(NANOS_OBJ_BUILD) $(AM_OBJ_BUILD)

$(BUILD):
	mkdir -p $(BUILD)
	
$(BOOT_BUILD_DIR):
	mkdir -p $(BOOT_BUILD_DIR)

$(USIM_BUILD_DIR):
	mkdir -p $(USIM_BUILD_DIR)

$(SYSCALL_BUILD_DIR):
	mkdir -p $(SYSCALL_BUILD_DIR)
	
$(NANOS_BUILD_DIR):
	mkdir -p $(NANOS_BUILD_DIR)

$(AM_BUILD_DIR):
	mkdir -p $(AM_BUILD_DIR)

$(USIM_OBJ_BUILD): $(USIM_BUILD_DIR)/%.o:$(USIM)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $^ -o $@

$(USIM_BUILD): $(USIM_OBJ_BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BOOT_OBJ_BUILD): $(BOOT_BUILD_DIR)/%.o:$(BOOT)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $^ -o $@
	$(OD) -D $@ > $@.dump

$(SYSCALL_OBJ_BUILD): $(SYSCALL_BUILD_DIR)/%.o:$(SYSCALL)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $^ -o $@
	$(OD) -D $@ > $@.dump

$(NANOS_OBJ_BUILD): $(NANOS_BUILD_DIR)/%.o:$(NANOS)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $^ -o $@
	$(OD) -D $@ > $@.dump

$(AM_OBJ_BUILD): $(AM_BUILD_DIR)/%.o:$(AM)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $^ -o $@
	$(OD) -D $@ > $@.dump

all: usim arm

arm: $(ALL_BUILD_DIR) $(ALL_OBJ)
	$(CC) -o $(BIN_BUILD) $(ALL_OBJ) $(LDFLAGS)
	$(OD) -D $(BIN_BUILD) > $(BIN_BUILD).dump
	$(OC) $(BIN_BUILD) -O binary $(BIN_BUILD).bin

usim: $(USIM_BUILD_DIR) $(USIM_BUILD)

utest: $(ALL_BUILD_DIR) $(USIM_BUILD) arm
	$(USIM_BUILD) $(BIN_BUILD).bin

clean:
	rm -r $(BUILD)