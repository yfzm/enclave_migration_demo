CC := /usr/local/popcorn/bin/clang
CFLAGS := -static -fPIC -nodefaultlibs -nostdlib -I./include -Wall -g
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

CC_AARCH64    := $(CC) -target aarch64-linux-gnu
BUILD_AARCH64 := ./aarch64
LIBS_AARCH64  := $(addprefix $(BUILD_AARCH64)/,$(LIBS))

###################################################################################
# x86-64
###################################################################################

CC_X86_64     := $(CC) -target x86_64-linux-gnu
BUILD_X86_64  := ./x86_64
LIBS_X86_64   := $(addprefix $(BUILD_X86_64)/,$(LIBS))

###################################################################################
# Recipes
###################################################################################

all:
	@$(CC) $(CFLAGS) -c stub.S
	@$(CC) $(CFLAGS) -c init.c
	@$(CC) $(CFLAGS) -c trampo.c
	@$(CC) $(CFLAGS) -c main.c
	@$(CC) $(CFLAGS) -c enclave_tls.c
	@$(CC) $(CFLAGS) -c ocall_syscall_wrapper.c
	@$(CC) $(CFLAGS) -c ocall_syscall.S
	@$(CC) $(CFLAGS) -c ocall_libcall_wrapper.c
	@$(CC) $(CFLAGS) -c migration.c
	@ld -T $(lds) -o enclave $(enclu_objs) $(app_objs) $(init_files) $(ocall_files) $(libc_files) $(migrate_files)
	@objdump -d enclave > enclave.asm

%/.dir:
	@echo " [MK] $*"
	@mkdir -p $*
	@touch $@

libs: aarch64 x86_64
	  
x86_64: $(BUILD_X86_64)/.dir $(LIBS_X86_64)

aarch64: $(BUILD_AARCH64)/.dir $(LIBS_AARCH64) 

$(BUILD_AARCH64)/%.o: %.c $(LIB_HDR)
	@$(CC_AARCH64) $(CFLAGS) -o $@ -c $<

$(BUILD_AARCH64)/%.o: %.S $(LIB_HDR)
	@$(CC_AARCH64) $(CFLAGS) -o $@ -c $<

$(BUILD_X86_64)/%.o: %.c $(LIB_HDR)
	@$(CC_X86_64) $(CFLAGS) -o $@ -c $<

$(BUILD_X86_64)/%.o: %.S $(LIB_HDR)
	@$(CC_X86_64) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.asm *.o enclave

.PHONY: all libs x86_64 arm clean
