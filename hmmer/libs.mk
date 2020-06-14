POPCORN := /usr/local/popcorn
CC      := $(POPCORN)/bin/clang
#CFLAGS := -static -fPIC -nodefaultlibs -nostdlib -I./include -Wall -g
CFLAGS := -O2 -mllvm -optimize-regalloc -Wall -g -popcorn-alignment \
	      -Wno-unused-variable -D_GNU_SOURCE -mllvm -no-sm-warn \
		  -I./include -static -fPIC -nostdlib -nodefaultlibs \
		  -DSPEC_CPU -DNDEBUG -DSPEC_CPU_LP64 
lds := linker.lds

LIB_HDR := $(shell ls *.h)
LIBS    := alphabet.o core_algorithms.o debug.o display.o emit.o emulation.o fast_algorithms.o histogram.o hmmio.o hmmcalibrate.o mathsupport.o masks.o misc.o modelmakers.o plan7.o plan9.o postprob.o prior.o tophits.o trace.o ucbqsort.o a2m.o aligneval.o alignio.o clustal.o cluster.o dayhoff.o eps.o file.o getopt.o gki.o gsi.o hsregex.o iupac.o msa.o msf.o phylip.o revcomp.o rk.o selex.o seqencode.o shuffle.o sqerror.o sqio.o squidcore.o sre_ctype.o sre_math.o sre_random.o sre_string.o ssi.o stack.o stockholm.o translate.o types.o vectorops.o weight.o
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
