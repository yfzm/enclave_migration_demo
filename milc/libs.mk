POPCORN := /usr/local/popcorn
CC      := $(POPCORN)/bin/clang
#CFLAGS := -static -fPIC -nodefaultlibs -nostdlib -I./include -Wall -g
CFLAGS := -O2 -mllvm -optimize-regalloc -Wall -g -popcorn-alignment \
	      -Wno-unused-variable -D_GNU_SOURCE -mllvm -no-sm-warn \
		  -I./include -static -fPIC -nostdlib -nodefaultlibs \
		  -DSPEC_CPU -DNDEBUG -DSPEC_CPU_LP64 \
		  -DFN -DFAST -DCONGRAD_TMP_VECTORS -DDSLASH_TMP_LINKS
lds := linker.lds

LIB_HDR := $(shell ls *.h)
LIBS    := f_meas.o gauge_info.o setup.o update.o update_h.o update_u.o layout_hyper.o check_unitarity.o d_plaq4.o gaugefix2.o io_helpers.o io_lat4.o make_lattice.o path_product.o ploop3.o ranmom.o ranstuff.o reunitarize2.o gauge_stuff.o grsource_imp.o mat_invert.o quark_stuff.o rephase.o cmplx.o addmat.o addvec.o clear_mat.o clearvec.o m_amatvec.o m_mat_an.o m_mat_na.o m_mat_nn.o m_matvec.o make_ahmat.o rand_ahmat.o realtr.o s_m_a_mat.o s_m_a_vec.o s_m_s_mat.o s_m_vec.o s_m_mat.o su3_adjoint.o su3_dot.o su3_rdot.o su3_proj.o su3mat_copy.o submat.o subvec.o trace_su3.o uncmp_ahmat.o msq_su3vec.o sub4vecs.o m_amv_4dir.o m_amv_4vec.o m_mv_s_4dir.o m_su2_mat_vec_n.o l_su2_hit_n.o r_su2_hit_a.o m_su2_mat_vec_a.o gaussrand.o byterevn.o m_mat_hwvec.o m_amat_hwvec.o dslash_fn2.o d_congrad5_fn.o com_vanilla.o io_nonansi.o
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
