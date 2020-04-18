.extern unsigned long register_frame_start;
.extern unsigned long register_frame_end;
.extern unsigned long previous_stack;
.extern unsigned long outside_stack;
.extern unsigned long outside_tramp;

.section .text.ocall_syscall, "ax"
.global ocall_syscall
.type ocall_syscall, @function

ocall_syscall:
// save the execution states (all the general purpose registers) in the enclave 
mrs x9, tpidr_el0               // thread pointer
mov x10, sp                     // tls: previous_stack <- sp
str x10, [x9, 48]
ldr x10, [x9, 16] 
mov sp, x10                     // tls: sp <- register_frame_start

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

mrs x9, tpidr_el0               // thread pointer
ldr x10, [x9, 32]
mov sp, x10                     // tls: sp <- outside_stack
ldr x9, =outside_tramp
ldr x10, [x9]
br x10

//[!!! exit trustzone]

// mov %fs:32, %rsp 				#tls: mov outside_stack, %rsp
// mov outside_tramp, %rbx
// mov $0x4, %rax					#EEXIT
// 
b eexit_tag
// #enclu

.section .text.ocall_return, "ax"
.global ocall_return
.type ocall_return, @function

ocall_return:
// restore the execution states before the ocall
mrs x9, tpidr_el0               // thread pointer
ldr x10, [x9, 16]
mov sp, x10                     // tls: sp <- register_frame_end

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

mrs x9, tpidr_el0               // thread pointer
ldr x10, [x9, 48]
mov sp, x10                     // tls: sp <- previous_stack
ret
