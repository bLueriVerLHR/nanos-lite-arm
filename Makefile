# COMPILER_DIR:= gcc-arm-none-eabi-10.3-2021.10/bin/
PREFIX 		:= arm-none-eabi-

CC := $(COMPILER_DIR)$(PREFIX)gcc
OC := $(COMPILER_DIR)$(PREFIX)objcopy
OD := $(COMPILER_DIR)$(PREFIX)objdump
RE := $(COMPILER_DIR)$(PREFIX)readelf

BIN 			:= armos
BUILD 			:= build
BIN_BUILD 		:= $(BUILD)/$(BIN)
ABS_BUILD_DIR 	:= $(CURDIR)/$(BUILD)

LDDIR		:= ldscripts
LDFILES		:= $(shell find $(LDDIR) -name '*.ld')

# C sources are compiled in cortex-m0 env
COMMONFLAGS := -mcpu=cortex-m0 -mlittle-endian
CPPFLAGS 	:= -DDEBUG_MODE
CFLAGS 		:= $(COMMONFLAGS) -nostartfiles -mthumb -Os
LDFLAGS 	:= $(COMMONFLAGS) -nostartfiles --specs=nano.specs # $(addprefix -T,$(LDFILES)) # -nostdlib

# C++ sources are compiled in native env
CXXFLAGS	+= -std=c++2a


BOOT			:= boot
BOOT_BUILD_DIR	:= $(ABS_BUILD_DIR)/$(BOOT)
BOOT_SRC 		:= $(shell find $(BOOT) -name '*.c')
BOOT_SRC_NODIR 	:= $(notdir $(BOOT_SRC))
BOOT_OBJ 		:= $(patsubst %.c,%.o,$(BOOT_SRC_NODIR))
BOOT_OBJ_BUILD 	:= $(addprefix $(BOOT_BUILD_DIR)/,$(BOOT_OBJ))

SIM := sim/build/sim

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

$(SYSCALL_BUILD_DIR):
	mkdir -p $(SYSCALL_BUILD_DIR)
	
$(NANOS_BUILD_DIR):
	mkdir -p $(NANOS_BUILD_DIR)

$(AM_BUILD_DIR):
	mkdir -p $(AM_BUILD_DIR)

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
	$(RE) -a $(BIN_BUILD) > $(BIN_BUILD).info
	$(OC) $(BIN_BUILD) -O binary $(BIN_BUILD).bin

$(SIM):
	$(MAKE) -C sim

utest: $(ALL_BUILD_DIR) $(SIM) arm
	$(SIM) $(BIN_BUILD).bin

clean:
	rm -r $(BUILD)