.section .bss
buffer:
	.space 2000 * 4

.section .rodata
percent_d:
	.asciz "%d\n"

.section .text
.globl main
main:
	# Align stack to 16 bytes.
	andq	$-16, %rsp

	# Read input:
	# %rbx = pointer to next element
	leaq	buffer-4(%rip), %rbx
1:	addq	$4, %rbx
	xorl	%eax, %eax
	leaq	percent_d(%rip), %rdi
	movq	%rbx, %rsi
	call	scanf
	test	%eax, %eax
	jns	1b                     # Done if scanf returns <0

	# Preserve the following:
	leaq	buffer(%rip), %r13     # %r13 = start of buffer
	movq	%rbx, %r15
	subq	%r13, %r15
	shrl	$2, %r15d              # %r15d = number of elements

	# Part 1:
	xorl	%ebx, %ebx             # %ebx = result
	movq	%r13, %rsi             # %rsi = pointer to first element
	leal	-1(%r15d), %ecx        # Loop counter
1:	lodsl                          # Load first + increment
	cmpl	(%rsi), %eax           # Compare with second element
	jge	2f
	incl	%ebx
2:	decl	%ecx
	jnz	1b

	# Print result.
	xorl	%eax, %eax
	leaq	percent_d(%rip), %rdi
	movl	%ebx, %esi
	call	printf

	# Part 2:
	xorl	%ebx, %ebx             # %ebx = result
	movq	%r13, %rsi             # %rsi = pointer to first element
	leal	-3(%r15d), %ecx        # Loop counter
1:	lodsl                          # Load first + increment
	cmpl	8(%rsi), %eax          # Compare with fourth element
	jge	2f
	incl	%ebx
2:	decl	%ecx
	jnz	1b

	# Print result.
	xorl	%eax, %eax
	leaq	percent_d(%rip), %rdi
	movl	%ebx, %esi
	call	printf

	# exit(0)
	xorl	%edi, %edi
	movl	$60, %eax
	syscall
