ARCH	:= CM0

BIN 		:= sim
ARCH_MACRO 	:= ARCH_$(ARCH)
DEBUG_MODE 	:= DEBUG_MODE

CXXINC 	:= include
CXXLIBS	:= m pthread

BUILD	:= build

MACROS 	:= $(ARCH_MACRO) $(DEBUG_MODE)

COMFLAGS	+= 
CPPFLAGS	+= $(COMFLAGS) $(addprefix -D,$(MACROS)) $(addprefix -I,$(CXXINC))
CXXFLAGS	+= $(COMFLAGS) -std=c++2a
LDFLAGS		+= $(COMFLAGS) $(addprefix -l,$(CXXLIBS))

SRCDIR		:= src

COMMONDIR	:= common
CXX_COMMON_SRC 		:= $(shell find $(SRCDIR)/$(COMMONDIR) -maxdepth 1 -name "*.cc")
CXX_COMMON_OBJ_DIR	:= $(patsubst %.cc,%.o,$(CXX_COMMON_SRC))
CXX_COMMON_OBJ_BUILD:= $(addprefix $(BUILD)/,$(CXX_COMMON_OBJ_DIR))

CPUDIR		:= cpu
CXX_CPU_SRC 		:= $(shell find $(SRCDIR)/$(CPUDIR) -maxdepth 1 -name "*.cc")
CXX_CPU_OBJ_DIR		:= $(patsubst %.cc,%.o,$(CXX_CPU_SRC))
CXX_CPU_OBJ_BUILD	:= $(addprefix $(BUILD)/,$(CXX_CPU_OBJ_DIR))

BUSDIR		:= bus
CXX_BUS_SRC 		:= $(shell find $(SRCDIR)/$(BUSDIR) -maxdepth 1 -name "*.cc")
CXX_BUS_OBJ_DIR		:= $(patsubst %.cc,%.o,$(CXX_BUS_SRC))
CXX_BUS_OBJ_BUILD	:= $(addprefix $(BUILD)/,$(CXX_BUS_OBJ_DIR))

all: param build

param:
	@echo CPPFLAGS=$(CPPFLAGS)
	@echo CXXFLAGS=$(CXXFLAGS)
	@echo LDFLAGS=$(LDFLAGS)
	@echo CXX_COMMON_SRC=$(CXX_COMMON_SRC)
	@echo CXX_COMMON_OBJ_DIR=$(CXX_COMMON_OBJ_DIR)
	@echo CXX_COMMON_OBJ_BUILD=$(CXX_COMMON_OBJ_BUILD)

# rule for main files
pre_make_common:
	mkdir -p $(BUILD)/$(SRCDIR)/$(COMMONDIR)

$(CXX_COMMON_SRC): pre_make_common

$(CXX_COMMON_OBJ_BUILD): $(BUILD)/$(SRCDIR)/$(COMMONDIR)/%.o:$(SRCDIR)/$(COMMONDIR)/%.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $^ -o $@

# rule for cpu
pre_make_cpu:
	mkdir -p $(BUILD)/$(SRCDIR)/$(CPUDIR)

$(CXX_CPU_SRC): pre_make_cpu

$(CXX_CPU_OBJ_BUILD): $(BUILD)/$(SRCDIR)/$(CPUDIR)/%.o:$(SRCDIR)/$(CPUDIR)/%.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $^ -o $@

# rule for bus
pre_make_bus:
	mkdir -p $(BUILD)/$(SRCDIR)/$(BUSDIR)

$(CXX_BUS_SRC): pre_make_bus

$(CXX_BUS_OBJ_BUILD): $(BUILD)/$(SRCDIR)/$(BUSDIR)/%.o:$(SRCDIR)/$(BUSDIR)/%.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $^ -o $@

# rule for bin
$(BUILD)/$(BIN): $(CXX_COMMON_OBJ_BUILD) $(CXX_BUS_OBJ_BUILD) $(CXX_CPU_OBJ_BUILD)
	$(CXX) $(LDFLAGS) $^ -o $@

build: $(BUILD)/$(BIN)
	
clean:
	rm -rf $(BUILD)
	


