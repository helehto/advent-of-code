.section .rodata
percent_lu:
	.asciz "%lu\n"
percent_d_comma:
	.asciz "%d,"

.section .text
.globl main
main:
	# Align stack to 16 bytes.
	andq	$-16, %rsp

	# Stack layout:
	# (%rsp)  = buffer for input integer
	# 8(%rsp) = lanternfish count table
	subq	$80, %rsp

	# Zero lanternfish count.
	xorl	%eax, %eax
	movl	$9, %ecx
	leaq	8(%rsp), %rdi
	rep stosq

	# Read input:
1:	xorl	%eax, %eax
	leaq	percent_d_comma(%rip), %rdi
	movq	%rsp, %rsi
	call	scanf
	test	%eax, %eax
	jle	.Ldone_reading         # Done if scanf returns <=0
	movl	(%rsp), %eax
	movl	%eax, %eax             # Zero upper dword
	incq	8(%rsp, %rax, 8)
	jmp	1b

.Ldone_reading:
	leaq	8(%rsp), %r13          # Pointer to the table
	movl	$256, %r14d            # Loop counter

1:
	# Decrease all counts by 1 by moving the array down one index.
	movl	$8, %ecx
	movq	%r13, %rdi
	leaq	8(%r13), %rsi
	movq	(%r13), %rax           # Save fish with count 0
	rep movsq

	# Update fish with count 6 and 8.
	addq	%rax, 8*6(%r13)
	movq	%rax, 8*8(%r13)

	# Print result for part 1 if appropriate.
	cmpl	$(256 - 80 + 1), %r14d
	jne	2f
	movq	%r13, %rdi
	call	print_lanternfish_count

	# Loop.
2:	decl	%r14d
	jnz	1b

	# Print result for part 2.
	movq	%r13, %rdi
	call	print_lanternfish_count

	# exit(0)
	xorl	%edi, %edi
	movl	$60, %eax
	syscall

# void (uint64_t *array)
print_lanternfish_count:
	# Count total number of fish.
	xorl	%esi, %esi
	addq	0*8(%rdi), %rsi
	addq	1*8(%rdi), %rsi
	addq	2*8(%rdi), %rsi
	addq	3*8(%rdi), %rsi
	addq	4*8(%rdi), %rsi
	addq	5*8(%rdi), %rsi
	addq	6*8(%rdi), %rsi
	addq	7*8(%rdi), %rsi
	addq	8*8(%rdi), %rsi

	# Print result.
	xorl	%eax, %eax
	leaq	percent_lu(%rip), %rdi
	jmp	printf
