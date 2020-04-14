#!/bin/bash
set -e

APP=hello

popcorn_path="/usr/local/popcorn"
popcorn_bin="$popcorn_path/bin"

cmd=${1:-build}

build() {
    # Generate *.o
    $popcorn_bin/clang -O2 -popcorn-migratable -c ${APP}.c

    # Generate map.txt
    $popcorn_bin/ld.gold -L/usr/lib/gcc/x86_64-linux-gnu/5 -Map map_x86_64.txt \
            ${APP}_x86_64.o -o ${APP}_x86_64 \
            $popcorn_path/x86_64/lib/crt1.o \
            $popcorn_path/x86_64/lib/libc.a \
            $popcorn_path/x86_64/lib/libmigrate.a \
            $popcorn_path/x86_64/lib/libstack-transform.a \
            $popcorn_path/x86_64/lib/libelf.a \
            $popcorn_path/x86_64/lib/libc.a \
            $popcorn_path/x86_64/lib/libpthread.a \
            --start-group -lgcc -lgcc_eh --end-group

    $popcorn_bin/ld.gold -L/usr/lib/gcc-cross/aarch64-linux-gnu/5 -Map map_aarch64.txt \
            ${APP}_aarch64.o -o ${APP}_aarch64 \
            $popcorn_path/aarch64/lib/crt1.o \
            $popcorn_path/aarch64/lib/libc.a \
            $popcorn_path/aarch64/lib/libmigrate.a \
            $popcorn_path/aarch64/lib/libstack-transform.a \
            $popcorn_path/aarch64/lib/libelf.a \
            $popcorn_path/aarch64/lib/libc.a \
            $popcorn_path/aarch64/lib/libpthread.a \
            --start-group -lgcc -lgcc_eh --end-group

    # Generate aligned linker script
    $popcorn_bin/pyalign --compiler-inst $popcorn_path \
            --arm-bin ${APP}_aarch64 --arm-map map_aarch64.txt \
            --x86-bin ${APP}_x86_64 --x86-map map_x86_64.txt

    # Relink
    $popcorn_bin/ld.gold -L/usr/lib/gcc/x86_64-linux-gnu/5 -T linker_script_x86.x \
            ${APP}_x86_64.o -o final_${APP}_x86_64 \
            $popcorn_path/x86_64/lib/crt1.o \
            $popcorn_path/x86_64/lib/libc.a \
            $popcorn_path/x86_64/lib/libmigrate.a \
            $popcorn_path/x86_64/lib/libstack-transform.a \
            $popcorn_path/x86_64/lib/libelf.a \
            $popcorn_path/x86_64/lib/libc.a \
            $popcorn_path/x86_64/lib/libpthread.a \
            --start-group -lgcc -lgcc_eh --end-group

    $popcorn_bin/ld.gold -L/usr/lib/gcc-cross/aarch64-linux-gnu/5 -T linker_script_arm.x \
            ${APP}_aarch64.o -o final_${APP}_aarch64 \
            $popcorn_path/aarch64/lib/crt1.o \
            $popcorn_path/aarch64/lib/libc.a \
            $popcorn_path/aarch64/lib/libmigrate.a \
            $popcorn_path/aarch64/lib/libstack-transform.a \
            $popcorn_path/aarch64/lib/libelf.a \
            $popcorn_path/aarch64/lib/libc.a \
            $popcorn_path/aarch64/lib/libpthread.a \
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
