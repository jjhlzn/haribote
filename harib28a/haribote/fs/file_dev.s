	.file	"file_dev.c"
	.text
	.globl	_file_read
	.def	_file_read;	.scl	2;	.type	32;	.endef
_file_read:
LFB3:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$28, %esp
	.cfi_offset 7, -12
	.cfi_offset 6, -16
	.cfi_offset 3, -20
	movl	12(%ebp), %esi
	cmpl	$0, 20(%ebp)
	jle	L12
	movl	20(%ebp), %edi
L11:
	movl	12(%esi), %eax
	movl	$1024, %ecx
	cltd
	idivl	%ecx
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_bmap
	movl	$0, -28(%ebp)
	testl	%eax, %eax
	je	L3
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movzwl	44(%eax), %eax
	movl	%eax, (%esp)
	call	_bread
	testl	%eax, %eax
	movl	%eax, -28(%ebp)
	je	L4
L3:
	movl	12(%esi), %eax
	movl	$1024, %ecx
	cltd
	idivl	%ecx
	movl	12(%esi), %eax
	subl	%edx, %ecx
	cmpl	%ecx, %edi
	cmovle	%edi, %ecx
	addl	%ecx, %eax
	subl	%ecx, %edi
	cmpl	$0, -28(%ebp)
	movl	%eax, 12(%esi)
	jne	L5
	movl	16(%ebp), %eax
	movl	%ecx, %ebx
	addl	%eax, %ebx
	movl	%ebx, -28(%ebp)
	xorl	%ebx, %ebx
	jmp	L6
L5:
	movl	-28(%ebp), %eax
	addl	(%eax), %edx
	xorl	%eax, %eax
	movl	%edx, -32(%ebp)
L7:
	movl	16(%ebp), %ebx
	addl	%eax, %ebx
	movl	%ebx, %edx
	movl	%ecx, %ebx
	subl	%eax, %ebx
	testl	%ebx, %ebx
	jle	L24
	movl	-32(%ebp), %ebx
	movb	(%ebx,%eax), %bl
/APP
 # 20 "../include/asm/segment.h" 1
	movb %bl,%fs:(%edx)
 # 0 "" 2
/NO_APP
	incl	%eax
	jmp	L7
L24:
	xorl	%eax, %eax
	testl	%ecx, %ecx
	cmovns	%ecx, %eax
	addl	%eax, 16(%ebp)
	movl	-28(%ebp), %eax
	movl	%eax, (%esp)
	call	_brelse
	jmp	L9
L6:
	movl	-28(%ebp), %edx
	subl	%eax, %edx
	testl	%edx, %edx
	jle	L25
	leal	1(%eax), %edx
/APP
 # 20 "../include/asm/segment.h" 1
	movb %bl,%fs:(%eax)
 # 0 "" 2
/NO_APP
	movl	%edx, %eax
	jmp	L6
L25:
	xorl	%eax, %eax
	testl	%ecx, %ecx
	cmovns	%ecx, %eax
	addl	%eax, 16(%ebp)
L9:
	testl	%edi, %edi
	jne	L11
L4:
	movl	20(%ebp), %edx
	movl	8(%ebp), %eax
	subl	%edi, %edx
	cmpl	%edi, 20(%ebp)
	movl	$1, 36(%eax)
	movl	$-99, %eax
	cmovne	%edx, %eax
	jmp	L2
L12:
	xorl	%eax, %eax
L2:
	addl	$28, %esp
	popl	%ebx
	.cfi_restore 3
	popl	%esi
	.cfi_restore 6
	popl	%edi
	.cfi_restore 7
	popl	%ebp
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
LFE3:
	.globl	_file_write
	.def	_file_write;	.scl	2;	.type	32;	.endef
_file_write:
LFB4:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$44, %esp
	.cfi_offset 7, -12
	.cfi_offset 6, -16
	.cfi_offset 3, -20
	movl	12(%ebp), %eax
	movl	8(%ebp), %ebx
	testb	$4, 3(%eax)
	je	L27
	movl	4(%ebx), %edi
	jmp	L28
L27:
	movl	12(%ebp), %eax
	movl	12(%eax), %edi
L28:
	movl	$0, -28(%ebp)
L29:
	movl	-28(%ebp), %eax
	cmpl	20(%ebp), %eax
	jge	L33
	movl	%edi, %eax
	movl	$1024, %esi
	cltd
	idivl	%esi
	movl	%ebx, (%esp)
	movl	%eax, 4(%esp)
	movl	%edx, -36(%ebp)
	call	_create_block
	testl	%eax, %eax
	jne	L30
L33:
	movl	12(%ebp), %eax
	movl	$1, 8(%ebx)
	testb	$4, 3(%eax)
	jne	L32
	movl	12(%ebp), %eax
	movl	%edi, 12(%eax)
	movl	$1, 40(%ebx)
	jmp	L32
L30:
	movl	%eax, 4(%esp)
	movzwl	44(%ebx), %eax
	movl	%eax, (%esp)
	call	_bread
	testl	%eax, %eax
	movl	%eax, %ecx
	movl	%eax, -32(%ebp)
	je	L33
	movl	-36(%ebp), %edx
	movl	(%eax), %eax
	movb	$1, 11(%ecx)
	movl	20(%ebp), %ecx
	subl	-28(%ebp), %ecx
	subl	%edx, %esi
	addl	%edx, %eax
	cmpl	%ecx, %esi
	movl	%ecx, %edx
	cmovle	%esi, %edx
	addl	%edx, %edi
	cmpl	4(%ebx), %edi
	movl	%eax, -36(%ebp)
	jbe	L34
	movl	%edi, 4(%ebx)
	movb	$1, 51(%ebx)
L34:
	addl	%edx, -28(%ebp)
	xorl	%ecx, %ecx
L35:
	movl	16(%ebp), %esi
	movl	%edx, %eax
	subl	%ecx, %eax
	addl	%ecx, %esi
	testl	%eax, %eax
	jle	L43
/APP
 # 5 "../include/asm/segment.h" 1
	movb %fs:(%esi),%al
 # 0 "" 2
/NO_APP
	movl	-36(%ebp), %esi
	movb	%al, (%esi,%ecx)
	incl	%ecx
	jmp	L35
L43:
	movl	-32(%ebp), %eax
	xorl	%ecx, %ecx
	testl	%edx, %edx
	cmovns	%edx, %ecx
	addl	%ecx, 16(%ebp)
	movl	%eax, (%esp)
	call	_brelse
	jmp	L29
L32:
	movl	-28(%ebp), %edi
	orl	$-1, %eax
	testl	%edi, %edi
	cmovne	%edi, %eax
	addl	$44, %esp
	popl	%ebx
	.cfi_restore 3
	popl	%esi
	.cfi_restore 6
	popl	%edi
	.cfi_restore 7
	popl	%ebp
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
LFE4:
	.ident	"GCC: (GNU) 4.8.1"
	.def	_bmap;	.scl	2;	.type	32;	.endef
	.def	_bread;	.scl	2;	.type	32;	.endef
	.def	_brelse;	.scl	2;	.type	32;	.endef
	.def	_create_block;	.scl	2;	.type	32;	.endef
