#------------------------------------------------------------------------------
#                           ENVIRONMENT VARIABLES                              
#------------------------------------------------------------------------------

# Used by the build utils
export HARDWARE
export WORKSPACE
export TEMP_DIR

TEMP_DIR	= $(BUILD_DIR)/tmp
BINARY		= $(BUILD_DIR)/$(PROJECT_NAME).elf


define tcl_util
	xsct $(PLATFORM_TOOLS)/util.tcl					\
	     $(1)							\
	     $(WORKSPACE) $(WORKSPACE)/tmp/$(HARDWARE)
endef

ARCH = $(shell $(call read_file, $(TEMP_DIR)/arch.txt))

# Define the platform compiler switch
CFLAGS += -DXILINX_PLATFORM						\
	 -fdata-sections						\
	 -ffunction-sections 						\
	 -O2								\
	 -g3
# Xilinx's generated linker script path
LSCRIPT		:= $(BUILD_DIR)/app/src/lscript.ld
# Xilinx's generate bsp library path
LIB_PATHS	+= -L$(BUILD_DIR)/bsp/$(ARCH)/lib

PLATFORM_RELATIVE_PATH = $1
PLATFORM_FULL_PATH = $1

################|--------------------------------------------------------------
################|                   Zynq                                       
################|--------------------------------------------------------------
ifeq (ps7_cortexa9_0,$(strip $(ARCH)))

CC := arm-none-eabi-gcc
AR := arm-none-eabi-ar

LD := $(CC)

CFLAGS += -mcpu=cortex-a9 						\
	  -mfpu=vfpv3 							\
	  -mfloat-abi=hard

LDFLAGS += -specs=$(BUILD_DIR)/app/src/Xilinx.spec 			\
	   -mfpu=vfpv3							\
 	   -mfloat-abi=hard 						\
	   -mcpu=cortex-a9						\
	   -Wl,-build-id=none

endif

################|--------------------------------------------------------------
################|                   ZynqMP                                     
################|--------------------------------------------------------------
ifeq (psu_cortexa53_0,$(strip $(ARCH)))

CC := aarch64-none-elf-gcc
AR := aarch64-none-elf-ar

LD := $(CC)

endif
################|--------------------------------------------------------------
################|                  Microblaze                                  
################|--------------------------------------------------------------
ifeq (sys_mb,$(strip $(ARCH)))

ifeq ($(OS), Windows_NT)
CC := mb-gcc
AR := mb-ar
else
CC := microblaze-xilinx-elf-gcc
AR := microblaze-xilinx-elf-ar
endif

LD := $(CC)

CFLAGS += -mcpu=cortex-a9 						\
	  -DXILINX -DMICROBLAZE						\
	  -DXILINX_PLATFORM						\
	  -mlittle-endian						\
	  -mxl-barrel-shift						\
	  -mxl-pattern-compare 						\
	  -mno-xl-soft-div						\
	  -mcpu=v11.0							\
	  -mno-xl-soft-mul						\
	  -mxl-multiply-high

LDFLAGS += -Xlinker --defsym=_HEAP_SIZE=0x100000 			\
	   -Xlinker --defsym=_STACK_SIZE=0x2000 			\
	   -mlittle-endian 						\
	   -mxl-barrel-shift						\
	   -mxl-pattern-compare						\
	   -mno-xl-soft-div						\
	   -mcpu=v11.0							\
	   -mno-xl-soft-mul						\
	   -mxl-multiply-high 						\
	   -Wl,--no-relax 						\
	   -Wl,--gc-sections 

endif

# Common xilinx libs
LIB_FLAGS += -Wl,--start-group,-lxil,-lgcc,-lc,--end-group

# Add the common include paths
CFLAGS += -I$(BUILD_DIR)/app/src
# Xilinx's bsp include path
CFLAGS		+= -I$(BUILD_DIR)/bsp/$(ARCH)/include

#Add more dependencies to $(BINARY) rule.
$(BINARY): $(TEMP_DIR)/arch.txt

PHONY += xilinx_run
xilinx_run: all
	$(MUTE) xsdb $(PLATFORM_TOOLS)/upload.tcl\
		$(ARCH)\
		$(BUILD_DIR)/hw/system_top.bit\
		$(BINARY)\
		$(BUILD_DIR)/hw $(HIDE)

$(TEMP_DIR)/arch.txt: $(HARDWARE)
	$(MUTE) $(call mk_dir,$(BUILD_DIR)/app $(BUILD_DIR)/app/src $(OBJECTS_DIR) $(TEMP_DIR)) $(HIDE)
	$(MUTE) $(call copy_fun,$(HARDWARE),$(TEMP_DIR)) $(HIDE)
	$(call print,Evaluating hardware: $(HARDWARE))
	$(MUTE)	$(call tcl_util, get_arch) $(HIDE)

xilinx_project: $(BUILD_DIR)/.bsp.target

PHONY += xilinx_project_build
xilinx_project_build: all
	$(MUTE) $(MUTE) xsct $(NO-OS)/tools/scripts/platform/xilinx/build_project.tcl $(WORKSPACE) $(HIDE)


$(BUILD_DIR)/.bsp.target: $(LIB_TARGETS) $(TEMP_DIR)/arch.txt
	$(call print,Creating and configuring the IDE project)
	$(MUTE)	$(call tcl_util, create_project)  $(HIDE)
	$(MUTE) $(MAKE) --no-print-directory update_srcs
	$(MUTE) $(call set_one_time_rule,$@) $(HIDE)

clean_all: xilinx_clean_all

xilinx_clean_all:
	-$(MUTE) $(call remove_dir,.Xil) $(HIDE)