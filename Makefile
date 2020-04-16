POPCORN := /usr/local/popcorn
CC      := $(POPCORN)/bin/clang
#CFLAGS := -static -fPIC -nodefaultlibs -nostdlib -I./include -Wall -g
CFLAGS := -O0 -mllvm -optimize-regalloc -Wall -g -popcorn-alignment \
	      -Wno-unused-variable -D_GNU_SOURCE -mllvm -no-sm-warn \
		  -I./include -static -fPIC -nostdlib -nodefaultlibs
lds := linker.lds

LIB_HDR := $(shell ls include/*.h)
LIBS    := stub.o init.o trampo.o enclave_tls.o ocall_syscall_wrapper.o \
	       ocall_syscall.o ocall_libcall_wrapper.o migration.o

init_files := init.o enclave_tls.o
libc_files := ./build/libc.a
ocall_files := ocall_libcall_wrapper.o ocall_syscall_wrapper.o 
enclu_objs := stub.o ocall_syscall.o 
migrate_files := migration.o
app_objs := trampo.o main.o

###################################################################################
# aarch64
###################################################################################

AARCH64       := aarch64
CC_AARCH64    := $(CC) -target aarch64-linux-gnu
BUILD_AARCH64 := build/aarch64
LIBS_AARCH64  := $(addprefix $(BUILD_AARCH64)/,$(LIBS))
INC_AARCH64   := -I$(POPCORN)/$(AARCH64)/include

###################################################################################
# x86-64
###################################################################################

X86_64        := x86_64
CC_X86_64     := $(CC) -target x86_64-linux-gnu
BUILD_X86_64  := build/x86_64
LIBS_X86_64   := $(addprefix $(BUILD_X86_64)/,$(LIBS))
INC_X86_64    := -I$(POPCORN)/$(X86_64)/include

###################################################################################
# Recipes
###################################################################################

all: libs
	@./build.sh

%/.dir:
	@echo " [MK] $*"
	@mkdir -p $*
	@touch $@

libs: aarch64 x86_64
	  
x86_64: $(BUILD_X86_64)/.dir $(LIBS_X86_64)

aarch64: $(BUILD_AARCH64)/.dir $(LIBS_AARCH64) 

$(BUILD_AARCH64)/%.o: %.c $(LIB_HDR)
	@$(CC_AARCH64) $(CFLAGS) $(INC_AARCH64) -o $@ -c $<

$(BUILD_AARCH64)/%.o: $(AARCH64)/%.S $(LIB_HDR)
	@$(CC_AARCH64) $(CFLAGS) $(INC_AARCH64) -o $@ -c $<

$(BUILD_X86_64)/%.o: %.c $(LIB_HDR)
	@$(CC_X86_64) $(CFLAGS) $(INC_X86_64) -o $@ -c $<

$(BUILD_X86_64)/%.o: $(X86_64)/%.S $(LIB_HDR)
	@$(CC_X86_64) $(CFLAGS) $(INC_X86_64) -o $@ -c $<

clean:
	./build.sh clean
	# rm -f *.asm *.o enclave
	rm -rf ./$(BUILD_X86_64) ./$(BUILD_AARCH64)

.PHONY: all libs x86_64 arm clean
