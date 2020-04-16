	.file	"init.c"
	.comm	outside_tramp,8,8
	.comm	fake_heap,8,8
	.text
	.globl	init_syscall
	.type	init_syscall, @function
init_syscall:
.LFB1:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movq	%rdi, -40(%rbp)
	movq	-40(%rbp), %rax
	movq	24(%rax), %rax
	movq	%rax, -8(%rbp)
	cmpq	$0, -8(%rbp)
	jne	.L2
	movq	-40(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, outside_tramp(%rip)
	movq	-40(%rbp), %rax
	movq	40(%rax), %rax
	movq	%rax, fake_heap(%rip)
	movq	-40(%rbp), %rax
	movq	48(%rax), %rax
	movq	%rax, mcode_pages(%rip)
	movq	-40(%rbp), %rax
	movq	56(%rax), %rax
	movq	%rax, mdata_pages(%rip)
	movq	-40(%rbp), %rax
	movq	64(%rax), %rax
	movq	%rax, mheap_pages(%rip)
	movq	-40(%rbp), %rax
	movq	72(%rax), %rax
	movq	%rax, mstack_pages(%rip)
	movq	-40(%rbp), %rax
	movq	80(%rax), %rax
	movq	%rax, mthread_pages(%rip)
	movq	$tls_1, -24(%rbp)
	movq	$init_stack_1, -16(%rbp)
	jmp	.L3
.L2:
	movq	-8(%rbp), %rdx
	movq	%rdx, %rax
	addq	%rax, %rax
	addq	%rdx, %rax
	salq	$12, %rax
	movq	%rax, %rdx
	movl	$tls_1, %eax
	addq	%rdx, %rax
	movq	%rax, -24(%rbp)
	movl	$init_stack_1, %edx
	movq	-8(%rbp), %rax
	imulq	$512000, %rax, %rax
	subq	%rax, %rdx
	movq	%rdx, %rax
	movq	%rax, -16(%rbp)
.L3:
	movq	-24(%rbp), %rdx
	movq	-24(%rbp), %rax
	movq	%rdx, (%rax)
	movq	-24(%rbp), %rax
	leaq	8(%rax), %rdx
	movq	-8(%rbp), %rax
	movq	%rax, (%rdx)
	movq	-24(%rbp), %rax
	addq	$16, %rax
	movq	-24(%rbp), %rdx
	addq	$4096, %rdx
	movq	%rdx, (%rax)
	movq	-24(%rbp), %rax
	addq	$24, %rax
	movq	-24(%rbp), %rdx
	addq	$3968, %rdx
	movq	%rdx, (%rax)
	movq	-24(%rbp), %rax
	leaq	32(%rax), %rdx
	movq	-40(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rax, (%rdx)
	movq	-24(%rbp), %rax
	leaq	40(%rax), %rdx
	movq	-40(%rbp), %rax
	movq	16(%rax), %rax
	movq	%rax, (%rdx)
	movq	-24(%rbp), %rax
	leaq	48(%rax), %rdx
	movq	-16(%rbp), %rax
	movq	%rax, (%rdx)
	movq	-24(%rbp), %rax
	leaq	56(%rax), %rdx
	movq	-40(%rbp), %rax
	movq	32(%rax), %rax
	movq	%rax, (%rdx)
	movq	-24(%rbp), %rax
	leaq	64(%rax), %rdx
	movq	-40(%rbp), %rax
	movq	88(%rax), %rax
	movq	%rax, (%rdx)
	nop
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	init_syscall, .-init_syscall
	.globl	my_start_libc
	.type	my_start_libc, @function
my_start_libc:
.LFB2:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, -4(%rbp)
	movq	%rsi, -16(%rbp)
	nop
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
	.size	my_start_libc, .-my_start_libc
	.globl	my_start
	.type	my_start, @function
my_start:
.LFB3:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	call	__tls_outside_buffer
	movq	(%rax), %rax
	addq	$4096, %rax
	movq	%rax, -16(%rbp)
	movq	-16(%rbp), %rax
	movq	(%rax), %rax
	movl	%eax, -20(%rbp)
	movq	-16(%rbp), %rax
	addq	$8, %rax
	movq	(%rax), %rax
	movq	%rax, -8(%rbp)
	movq	-8(%rbp), %rdx
	movl	-20(%rbp), %eax
	movq	%rdx, %rsi
	movl	%eax, %edi
	call	my_start_libc
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE3:
	.size	my_start, .-my_start
	.ident	"GCC: (Ubuntu 5.4.0-6ubuntu1~16.04.12) 5.4.0 20160609"
	.section	.note.GNU-stack,"",@progbits
