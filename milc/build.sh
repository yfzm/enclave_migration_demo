#!/bin/bash
set -e

APP=main
#CFLAGS='-I./include -I. -DTIMING_OUTPUT'
CFLAGS="-I./include -I. -DSPEC_CPU -DNDEBUG -DSPEC_CPU_LP64 -DFN -DFAST -DCONGRAD_TMP_VECTORS -DDSLASH_TMP_LINKS"

cmd=${1:-build}

popcorn_path="/usr/local/popcorn"
popcorn_bin="$popcorn_path/bin"
lib_build_path="../build"
lib_build_path_escaped=$(echo $lib_build_path | sed "s/\//\\\\\//g")

LIBS="f_meas.o gauge_info.o setup.o update.o update_h.o update_u.o layout_hyper.o check_unitarity.o d_plaq4.o gaugefix2.o io_helpers.o io_lat4.o make_lattice.o path_product.o ploop3.o ranmom.o ranstuff.o reunitarize2.o gauge_stuff.o grsource_imp.o mat_invert.o quark_stuff.o rephase.o cmplx.o addmat.o addvec.o clear_mat.o clearvec.o m_amatvec.o m_mat_an.o m_mat_na.o m_mat_nn.o m_matvec.o make_ahmat.o rand_ahmat.o realtr.o s_m_a_mat.o s_m_a_vec.o s_m_s_mat.o s_m_vec.o s_m_mat.o su3_adjoint.o su3_dot.o su3_rdot.o su3_proj.o su3mat_copy.o submat.o subvec.o trace_su3.o uncmp_ahmat.o msq_su3vec.o sub4vecs.o m_amv_4dir.o m_amv_4vec.o m_mv_s_4dir.o m_su2_mat_vec_n.o l_su2_hit_n.o r_su2_hit_a.o m_su2_mat_vec_a.o gaussrand.o byterevn.o m_mat_hwvec.o m_amat_hwvec.o dslash_fn2.o d_congrad5_fn.o com_vanilla.o io_nonansi.o"


lib_popcorn_path_escaped=$(echo $popcorn_path | sed "s/\//\\\\\//g")

popcorn_libs="crt1.o libc.a libmigrate.a libstack-transform.a libelf.a libc.a libpthread.a"
x86_popcorn_libs=$(echo "$popcorn_libs" | sed "s/[^ ]* */${lib_popcorn_path_escaped}\/x86_64\/lib\/&/g")
arm_popcorn_libs=$(echo "$popcorn_libs" | sed "s/[^ ]* */${lib_popcorn_path_escaped}\/aarch64\/lib\/&/g")

x86_enclave_objs="
    ${lib_build_path}/x86_64/stub.o
    ${lib_build_path}/x86_64/ocall_syscall.o
    ${lib_build_path}/x86_64/trampo.o
    ${APP}_x86_64.o
    $(echo "$LIBS" | sed "s/[^ ]* */${lib_build_path_escaped}\/x86_64\/&/g")
    ${lib_build_path}/x86_64/init.o
    ${lib_build_path}/x86_64/enclave_tls.o
    ${lib_build_path}/x86_64/ocall_libcall_wrapper.o
    ${lib_build_path}/x86_64/ocall_syscall_wrapper.o
    ${lib_build_path}/x86_64/migration.o
"


arm_enclave_objs="
    ${lib_build_path}/aarch64/stub.o
    ${lib_build_path}/aarch64/ocall_syscall.o
    ${lib_build_path}/aarch64/trampo.o
    ${APP}_aarch64.o
    $(echo "$LIBS" | sed "s/[^ ]* */${lib_build_path_escaped}\/aarch64\/&/g")
    ${lib_build_path}/aarch64/init.o
    ${lib_build_path}/aarch64/enclave_tls.o
    ${lib_build_path}/aarch64/ocall_libcall_wrapper.o
    ${lib_build_path}/aarch64/ocall_syscall_wrapper.o
    ${lib_build_path}/aarch64/migration.o
"

x86_objs="$x86_enclave_objs $x86_popcorn_libs"
arm_objs="$arm_enclave_objs $arm_popcorn_libs"


build() {
    # Make libs
    make -C ../libs

    # Generate *.o
    $popcorn_bin/clang $CFLAGS -O2 -popcorn-migratable -c ${APP}.c

    echo "Link (1/2) generate map files"
    # Generate map.txt
    $popcorn_bin/ld.gold -L/usr/lib/gcc/x86_64-linux-gnu/5 -Map map_x86_64.txt \
            -T ${lib_build_path}/x86_64/linker.lds \
            -o ${APP}_x86_64 \
            $x86_objs \
            --start-group -lgcc -lgcc_eh --end-group

    $popcorn_bin/ld.gold -L/usr/lib/gcc-cross/aarch64-linux-gnu/5 -Map map_aarch64.txt \
            -T ${lib_build_path}/aarch64/linker.lds \
            -o ${APP}_aarch64 \
            $arm_objs \
            --start-group -lgcc -lgcc_eh --end-group

    echo "Align"
    # Generate aligned linker script
    $popcorn_bin/pyalign --compiler-inst $popcorn_path \
            --arm-bin ${APP}_aarch64 --arm-map map_aarch64.txt \
            --x86-bin ${APP}_x86_64 --x86-map map_x86_64.txt

    echo "Link (2/2) generate final executables"
    # Relink
    $popcorn_bin/ld.gold -L/usr/lib/gcc/x86_64-linux-gnu/5 -T linker_script_x86.x \
            -o final_${APP}_x86_64 \
            $x86_objs \
            --start-group -lgcc -lgcc_eh --end-group

    $popcorn_bin/ld.gold -L/usr/lib/gcc-cross/aarch64-linux-gnu/5 -T linker_script_arm.x \
            -o final_${APP}_aarch64 \
            $arm_objs \
            --start-group -lgcc -lgcc_eh --end-group

    echo "Post process"
    # Post processing
    $popcorn_bin/gen-stackinfo -f final_${APP}_aarch64
    $popcorn_bin/gen-stackinfo -f final_${APP}_x86_64

    echo "Done"
}

nm() {
	make -C ../libs
	$popcorn_bin/clang $CFLAGS -DNO_MIGRATION -c ${APP}.c -o main_x86_64.o
    $popcorn_bin/ld.gold -L/usr/lib/gcc/x86_64-linux-gnu/5 \
            -T ${lib_build_path}/x86_64/linker.lds \
            -o ${APP}_nm_x86_64 \
            $x86_enclave_objs $popcorn_path/x86_64/lib/libc.a \
            --start-group -lgcc -lgcc_eh --end-group
}

clean() {
    rm -f final*
    rm -f *.x
    rm -f *.txt
    rm -f *.o
    rm -f *_x86_64
    rm -f *_aarch64
}

relink() {
    $popcorn_bin/ld.gold -L/usr/lib/gcc/x86_64-linux-gnu/5 -T linker_script_x86.x \
            -o final_${APP}_x86_64 \
            $x86_objs \
            --start-group -lgcc -lgcc_eh --end-group
    $popcorn_bin/gen-stackinfo -f final_${APP}_x86_64
}

$cmd
