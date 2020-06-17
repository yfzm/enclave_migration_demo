#!/bin/bash
set -e

APP=main
#CFLAGS='-I./include -I. -DTIMING_OUTPUT'
CFLAGS="-I./include -I. -DSPEC_CPU -DNDEBUG -DSPEC_CPU_LP64 -DSPEC_CPU_LINUX"

cmd=${1:-build}

popcorn_path="/usr/local/popcorn"
popcorn_bin="$popcorn_path/bin"
lib_build_path="../build"
lib_build_path_escaped=$(echo $lib_build_path | sed "s/\//\\\\\//g")

LIBS="classic.o complex.o decoherence.o expn.o gates.o matrix.o measure.o oaddn.o objcode.o omuln.o qec.o qft.o qureg.o version.o specrand.o"


x86_objs="
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
    $popcorn_path/x86_64/lib/crt1.o
    $popcorn_path/x86_64/lib/libc.a
    $popcorn_path/x86_64/lib/libmigrate.a
    $popcorn_path/x86_64/lib/libstack-transform.a
    $popcorn_path/x86_64/lib/libelf.a
    $popcorn_path/x86_64/lib/libc.a
    $popcorn_path/x86_64/lib/libpthread.a
"


arm_objs="
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
    $popcorn_path/aarch64/lib/crt1.o
    $popcorn_path/aarch64/lib/libc.a
    $popcorn_path/aarch64/lib/libmigrate.a
    $popcorn_path/aarch64/lib/libstack-transform.a
    $popcorn_path/aarch64/lib/libelf.a
    $popcorn_path/aarch64/lib/libc.a
    $popcorn_path/aarch64/lib/libpthread.a
"


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
