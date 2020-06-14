POPCORN := /usr/local/popcorn
CC      := $(POPCORN)/bin/clang
#CFLAGS := -static -fPIC -nodefaultlibs -nostdlib -I./include -Wall -g
CFLAGS := -O2 -mllvm -optimize-regalloc -Wall -g -popcorn-alignment \
	      -Wno-unused-variable -D_GNU_SOURCE -mllvm -no-sm-warn \
		  -I./include -static -fPIC -nostdlib -nodefaultlibs \
		  -DSPEC_CPU -DNDEBUG -DSPEC_CPU_LP64 -DPERL_CORE \
		  -DSPEC_CPU_NO_I_INTTYPES -DSPEC_CPU_NO_HAS_ISNAN \
		  -DSPEC_CPU_NO_I_SYS_STAT
lds := linker.lds

LIB_HDR := $(shell ls *.h)
LIBS    := av.o deb.o doio.o doop.o dump.o globals.o gv.o hv.o locale.o mg.o numeric.o op.o pad.o perl.o perlapi.o perlio.o perly.o pp.o pp_ctl.o pp_hot.o pp_pack.o pp_sort.o pp_sys.o regcomp.o regexec.o run.o scope.o sv.o taint.o toke.o universal.o utf8.o util.o xsutils.o Base64.o Cwd.o Dumper.o HiRes.o IO.o Peek.o attrs.o poll.o stdio.o DynaLoader.o MD5.o Storable.o Parser.o specrand.o Hostname.o Opcode.o
BUILD_PATH := ../build


###################################################################################
# aarch64
###################################################################################

AARCH64       := aarch64
CC_AARCH64    := $(CC) -target aarch64-linux-gnu
BUILD_AARCH64 := $(BUILD_PATH)/aarch64
LIBS_AARCH64  := $(addprefix $(BUILD_AARCH64)/,$(LIBS))
INC_AARCH64   := -I$(POPCORN)/$(AARCH64)/include

###################################################################################
# x86-64
###################################################################################

X86_64        := x86_64
CC_X86_64     := $(CC) -target x86_64-linux-gnu
BUILD_X86_64  := $(BUILD_PATH)/x86_64
LIBS_X86_64   := $(addprefix $(BUILD_X86_64)/,$(LIBS))
INC_X86_64    := -I$(POPCORN)/$(X86_64)/include

###################################################################################
# Recipes
###################################################################################

# all: aarch64 x86_64
# 	@#./build.sh

%/.dir:
	@echo " [MK] $*"
	@mkdir -p $*
	@touch $@

libs: aarch64 x86_64
	  
x86_64: $(BUILD_X86_64)/.dir $(LIBS_X86_64)
	@#cp $(X86_64)/linker.lds $(BUILD_X86_64)/linker.lds

aarch64: $(BUILD_AARCH64)/.dir $(LIBS_AARCH64) 
	@#cp $(AARCH64)/linker.lds $(BUILD_AARCH64)/linker.lds

$(BUILD_AARCH64)/%.o: %.c $(LIB_HDR)
	@$(CC_AARCH64) $(CFLAGS) $(INC_AARCH64) -o $@ -c $<

$(BUILD_AARCH64)/%.o: $(AARCH64)/%.S $(LIB_HDR)
	@$(CC_AARCH64) $(CFLAGS) $(INC_AARCH64) -o $@ -c $<

$(BUILD_X86_64)/%.o: %.c $(LIB_HDR)
	@$(CC_X86_64) $(CFLAGS) $(INC_X86_64) -o $@ -c $<

$(BUILD_X86_64)/%.o: $(X86_64)/%.S $(LIB_HDR)
	@$(CC_X86_64) $(CFLAGS) $(INC_X86_64) -o $@ -c $<

libs_clean:
	# ./build.sh clean
	# rm -f *.asm *.o enclave
	rm -f $(LIBS_X86_64) $(LIBS_AARCH64)

.PHONY: all libs x86_64 arm clean
