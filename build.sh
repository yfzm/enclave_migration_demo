#!/bin/bash
set -e

APP=main
CFLAGS='-I./include'

cmd=${1:-build}

popcorn_path="/usr/local/popcorn"
popcorn_bin="$popcorn_path/bin"

x86_objs="
    $popcorn_path/x86_64/lib/crt1.o
    $popcorn_path/x86_64/lib/libc.a
    $popcorn_path/x86_64/lib/libmigrate.a
    $popcorn_path/x86_64/lib/libstack-transform.a
    $popcorn_path/x86_64/lib/libelf.a
    $popcorn_path/x86_64/lib/libc.a
    $popcorn_path/x86_64/lib/libpthread.a
    build/x86_64/init.o
    build/x86_64/enclave_tls.o
    build/x86_64/ocall_libcall_wrapper.o
    build/x86_64/ocall_syscall_wrapper.o
    build/x86_64/stub.o
    build/x86_64/ocall_syscall.o
    build/x86_64/migration.o
    build/x86_64/trampo.o
"

arm_objs="
    $popcorn_path/aarch64/lib/crt1.o
    $popcorn_path/aarch64/lib/libc.a
    $popcorn_path/aarch64/lib/libmigrate.a
    $popcorn_path/aarch64/lib/libstack-transform.a
    $popcorn_path/aarch64/lib/libelf.a
    $popcorn_path/aarch64/lib/libc.a
    $popcorn_path/aarch64/lib/libpthread.a
    build/aarch64/init.o
    build/aarch64/enclave_tls.o
    build/aarch64/ocall_libcall_wrapper.o
    build/aarch64/ocall_syscall_wrapper.o
    build/aarch64/stub.o
    build/aarch64/ocall_syscall.o
    build/aarch64/migration.o
    build/aarch64/trampo.o
"

build() {
    # Make libs
    make libs

    # Generate *.o
    $popcorn_bin/clang $CFLAGS -O2 -popcorn-migratable -c ${APP}.c

    # Generate map.txt
    $popcorn_bin/ld.gold -L/usr/lib/gcc/x86_64-linux-gnu/5 -Map map_x86_64.txt \
            -T linker.lds \
            ${APP}_x86_64.o -o ${APP}_x86_64 \
            $x86_objs \
            --start-group -lgcc -lgcc_eh --end-group

    $popcorn_bin/ld.gold -L/usr/lib/gcc-cross/aarch64-linux-gnu/5 -Map map_aarch64.txt \
            -T linker.lds \
            ${APP}_aarch64.o -o ${APP}_aarch64 \
            $arm_objs \
            --start-group -lgcc -lgcc_eh --end-group

    # Generate aligned linker script
    $popcorn_bin/pyalign --compiler-inst $popcorn_path \
            --arm-bin ${APP}_aarch64 --arm-map map_aarch64.txt \
            --x86-bin ${APP}_x86_64 --x86-map map_x86_64.txt

    # Relink
    $popcorn_bin/ld.gold -L/usr/lib/gcc/x86_64-linux-gnu/5 -T linker_script_x86.x \
            ${APP}_x86_64.o -o final_${APP}_x86_64 \
            $x86_objs \
            --start-group -lgcc -lgcc_eh --end-group

    $popcorn_bin/ld.gold -L/usr/lib/gcc-cross/aarch64-linux-gnu/5 -T linker_script_arm.x \
            ${APP}_aarch64.o -o final_${APP}_aarch64 \
            $arm_objs \
            --start-group -lgcc -lgcc_eh --end-group

    # Post processing
    $popcorn_bin/gen-stackinfo -f final_${APP}_aarch64
    $popcorn_bin/gen-stackinfo -f final_${APP}_x86_64
}

clean() {
    rm final*
    rm *.x
    rm *.txt
    rm *.o
    rm *_x86_64
    rm *_aarch64
}

$cmd
