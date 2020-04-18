#include "function_table.h"
.extern init_stack

// stub (entry & exit): saving and restoring register
.section .text.enclave_start, "ax"
.global enclave_start
.type enclave_start, @function
enclave_start:
cmp x9, #INIT_SYSCALL  // init the enclave
bne entry

// use this enclave page as stack during initilization
mov x9, sp          // mov %rsp, %rax
// mov $0x18801000, %rsp
// mov $0x40801000, %rsp
ldr x10, =init_stack
mov sp, x10         // mov $init_stack, %rsp
// save the context
stp	x0, x1, [sp, #16 * 0]
stp	x2, x3, [sp, #16 * 1]
stp	x4, x5, [sp, #16 * 2]
stp	x6, x7, [sp, #16 * 3]
stp	x8, x9, [sp, #16 * 4]
stp	x10, x11, [sp, #16 * 5]
stp	x12, x13, [sp, #16 * 6]
stp	x14, x15, [sp, #16 * 7]
stp	x16, x17, [sp, #16 * 8]
stp	x18, x19, [sp, #16 * 9]
stp	x20, x21, [sp, #16 * 10]
stp	x22, x23, [sp, #16 * 11]
stp	x24, x25, [sp, #16 * 12]
stp	x26, x27, [sp, #16 * 13]
stp	x28, x29, [sp, #16 * 14]
stp	x30, x21, [sp, #16 * 15]    // save return address (rip)

// return address
adr x30, #exit

// mov x0, x9    // ??? why x9? first argument(passed by %r9): choose which function
// mov x1, x10   // second argument(passed by %r10)
b trampoline

entry:
## for ocall jump back to the enclave
## TODO: should check if there is a previous ocall. Each enclave should add a mark before ocall.
ldr x10, =SYSCALL_RET
cmp x0, x10
beq ocall_return

// [!!!!!] trampoline stack (outside_stack will continue use the original stack)
mrs x9, tpidr_el0     // thread pointer
mov x10, sp
str x10, [x9, 32]
ldr x11, [x9, 48]
mov sp, x11            // set new stack (retreive from TLS)
mov x9, x10

// save the context
stp	x0, x1, [sp, #16 * 0]
stp	x2, x3, [sp, #16 * 1]
stp	x4, x5, [sp, #16 * 2]
stp	x6, x7, [sp, #16 * 3]
stp	x8, x9, [sp, #16 * 4]
stp	x10, x11, [sp, #16 * 5]
stp	x12, x13, [sp, #16 * 6]
stp	x14, x15, [sp, #16 * 7]
stp	x16, x17, [sp, #16 * 8]
stp	x18, x19, [sp, #16 * 9]
stp	x20, x21, [sp, #16 * 10]
stp	x22, x23, [sp, #16 * 11]
stp	x24, x25, [sp, #16 * 12]
stp	x26, x27, [sp, #16 * 13]
stp	x28, x29, [sp, #16 * 14]
stp	x30, x21, [sp, #16 * 15]    // save return address (rip)

// return address
adr x30, #exit
// arguments
// mov x0, x9   // first argument(passed by %r9): choose which function
// mov x1, x10  // second argument(passed by %r10)
b trampoline

exit:
## restore the context
ldp	x30, x21, [sp, #16 * 15] 
ldp	x0, x1, [sp, #16 * 0]
ldp	x2, x3, [sp, #16 * 1]
ldp	x4, x5, [sp, #16 * 2]
ldp	x6, x7, [sp, #16 * 3]
ldp	x8, x9, [sp, #16 * 4]
ldp	x10, x11, [sp, #16 * 5]
ldp	x12, x13, [sp, #16 * 6]
ldp	x14, x15, [sp, #16 * 7]
ldp	x16, x17, [sp, #16 * 8]
ldp	x18, x19, [sp, #16 * 9]
ldp	x20, x21, [sp, #16 * 10]
ldp	x22, x23, [sp, #16 * 11]
ldp	x24, x25, [sp, #16 * 12]
ldp	x26, x27, [sp, #16 * 13]
ldp	x28, x29, [sp, #16 * 14]

mov sp, x9   // the old rsp is saved in x0, acctually EEXIT will restore RSP & RBP

// eexit
// mov %rcx, %rbx #after eenter, rcx holds the next IP.
// mov $0x405000, %rcx
// mov $0x4, %rax #EEXIT will not restore the RSP
// 
.global eexit_tag
eexit_tag:
ret
// enclu 
// nop
// nop
// nop
// nop
// nop
// nop
// nop
// nop
// nop
// nop
// nop
// nop
// nop
// nop

start:
//followed by in-enclave trampoline function
