.section .rodata
forward_fmt:
	.asciz "forward %d\n"
up_fmt:
	.asciz "up %d\n"
down_fmt:
	.asciz "down %d\n"
percent_d:
	.asciz "%d\n"

.section .text
.globl main
main:
	# Align stack to 16 bytes.
	andq	$-16, %rsp

	# %r12d = x
	# %r13d = depth (part 1) / aim (part 2)
	# %r14d = depth (part 2)
	xorl	%r12d, %r12d
	xorl	%r13d, %r13d
	xorl	%r14d, %r14d

.Lloop:
	# Check for "forward %d"
	xorl	%eax, %eax
	leaq	forward_fmt(%rip), %rdi
	movq	%rsp, %rsi
	call	scanf
	testl	%eax, %eax
	js	.Lend                   # Done if scanf returns <0
	jnz	.Lforward

	# Check for "up %d"
	xorl	%eax, %eax
	leaq	up_fmt(%rip), %rdi
	movq	%rsp, %rsi
	call	scanf
	testl	%eax, %eax
	jnz	.Lup

	# Check for "down %d"
	xorl	%eax, %eax
	leaq	down_fmt(%rip), %rdi
	movq	%rsp, %rsi
	call	scanf
	testl	%eax, %eax
	jnz	.Ldown

.Lforward:
	addl	(%rsp), %r12d
	movl	%r13d, %eax
	mull	(%rsp)
	addl	%eax, %r14d
	jmp	.Lloop

.Lup:
	subl	(%rsp), %r13d
	jmp	.Lloop

.Ldown:
	addl	(%rsp), %r13d
	jmp	.Lloop

.Lend:
	# Print result for part 1.
	movl	%r12d, %eax
	mull	%r13d
	movl	%eax, %esi
	xorl	%eax, %eax
	leaq	percent_d(%rip), %rdi
	call	printf

	# Print result for part 2.
	movl	%r12d, %eax
	mull	%r14d
	movl	%eax, %esi
	xorl	%eax, %eax
	leaq	percent_d(%rip), %rdi
	call	printf

	# exit(0)
	xorl	%edi, %edi
	movl	$60, %eax
	syscall
