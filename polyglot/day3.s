#
# Look ma, no libc!
# (Only part 2, I'm too lazy to do part 1)
#

.section .bss
buffer:
	.space 65536
buffer_end:

.section .text
.globl _start
_start:
	xorl	%edi, %edi             # fd 0 is stdin
	movq	$buffer, %rsi          # buffer
	movl	$65536, %edx           # length
	xorl	%eax, %eax             # read()
	syscall

	movq	$buffer, %rdi          # %rdi is the input pointer
	xorl	%ebp, %ebp             # %ebp is the result (sum)

.Lmainloop:
	cmpb	$0x20, (%rdi)          # is *%rdi a control character?
	jb	.Ldone                 # if so, we are done

	# read a group of 3 and figure out the common bit
	callq	read_bitmask_from_line
	movq	%rax, %r15
	callq	read_bitmask_from_line
	andq	%rax, %r15
	callq	read_bitmask_from_line
	andq	%rax, %r15
	tzcntq	%r15, %rax
	addl	%eax, %ebp

	jmp	.Lmainloop

.Ldone:
	# Print the answer
	movl	%ebp, %edi
	callq	print_u32

	xorl	%edi, %edi             # return code 0
	movl	$60, %eax              # exit()
	syscall

print_u32:
	movl	%edi, %r8d
	movq	%rsp, %rbp
	movl	$10, %ecx              # divisor

	decq	%rsp
	movb	$'\n', (%rsp)          # add ending newline

.Ldivloop:
	movl	%r8d, %eax
	xorl	%edx, %edx
	divl	%ecx
	addl	$'0', %edx
	decq	%rsp
	movb	%dl, (%rsp)
	movl	%eax, %r8d
	testl	%r8d, %r8d
	jnz	.Ldivloop

	movq	%rsp, %rsi             # buffer
	movq	%rbp, %rdx
	subq	%rsp, %rdx             # length = end - p
	movl	$1, %edi               # fd 1 is stdout
	movl	$1, %eax               # write()
	syscall

	movq	%rbp, %rsp
	ret

read_bitmask_from_line:
	xorl	%eax, %eax
	movq	%rcx, %rdx             # preserve %rcx

.Lcharloop:
	movzbl	(%rdi), %ebx
	incq	%rdi

	# compare against 'a'..'z':
	movl	%ebx, %ecx
	subl	$('a' - 1), %ecx
	cmpl	$26, %ecx
	jae	1f

	# it's 'a'..'z': set bit 1..26
	movl	$1, %ebx
	shlq	%cl, %rbx
	orq	%rbx, %rax
	jmp	2f

1:	# it's 'A'..'Z': set bit 27..52
	movl	%ebx, %ecx
	subl	$('A' - 1), %ecx
	movl	$1, %ebx
	addl	$26, %ecx
	shlq	%cl, %rbx
	orq	%rbx, %rax

2:	cmpb	$'\n', (%rdi)
	jne	.Lcharloop
	incq	%rdi # skip '\n'

	movq	%rdx, %rcx             # restore %rcx
	ret
