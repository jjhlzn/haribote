; naskfunc
; TAB=4

[FORMAT "WCOFF"]				; �I�u�W�F�N�g�t�@�C������郂�[�h	
[INSTRSET "i486p"]				; 486�̖��߂܂Ŏg�������Ƃ����L�q
[BITS 32]						; 32�r�b�g���[�h�p�̋@�B�����点��
[FILE "naskfunc.nas"]			; �\�[�X�t�@�C�������

		GLOBAL	_io_hlt, _io_cli, _io_sti, _io_stihlt
		GLOBAL	_io_in8,  _io_in16,  _io_in32
		GLOBAL	_io_out8, _io_out16, _io_out32
		GLOBAL	_io_load_eflags, _io_store_eflags
		GLOBAL	_load_gdtr, _load_idtr
		GLOBAL	_load_cr0, _store_cr0
		GLOBAL	_load_tr
		GLOBAL	_asm_inthandler20, _asm_inthandler21
		GLOBAL	_asm_inthandler2c, _asm_inthandler0c
		GLOBAL  _asm_inthandler2e
		GLOBAL	_asm_inthandler0d, _asm_end_app
		GLOBAL	_memtest_sub
		GLOBAL	_farjmp, _farcall
		GLOBAL	_asm_hrb_api, _start_app
		GLOBAL  _disable_irq, _enable_irq, _port_read,_port_write
		GLOBAL  _memcpy1, _memset1,_ud2
		EXTERN	_inthandler20, _inthandler21
		EXTERN	_inthandler2c, _inthandler0d
		EXTERN 	_inthandler2e
		EXTERN	_inthandler0c
		EXTERN	_hrb_api

[SECTION .text]

_ud2:
	DB 0x0f, 0x0b
	RET

_io_hlt:	; void io_hlt(void);
		HLT
		RET

_io_cli:	; void io_cli(void);
		CLI
		RET

_io_sti:	; void io_sti(void);
		STI
		RET

_io_stihlt:	; void io_stihlt(void);
		STI
		HLT
		RET

_io_in8:	; int io_in8(int port);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,0
		IN		AL,DX
		RET

_io_in16:	; int io_in16(int port);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,0
		IN		AX,DX
		RET

_io_in32:	; int io_in32(int port);
		MOV		EDX,[ESP+4]		; port
		IN		EAX,DX
		RET

_io_out8:	; void io_out8(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		AL,[ESP+8]		; data
		OUT		DX,AL
		RET

_io_out16:	; void io_out16(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,[ESP+8]		; data
		OUT		DX,AX
		RET

_io_out32:	; void io_out32(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,[ESP+8]		; data
		OUT		DX,EAX
		RET

_io_load_eflags:	; int io_load_eflags(void);
		PUSHFD		; PUSH EFLAGS �Ƃ����Ӗ�
		POP		EAX
		RET

_io_store_eflags:	; void io_store_eflags(int eflags);
		MOV		EAX,[ESP+4]
		PUSH	EAX
		POPFD		; POP EFLAGS �Ƃ����Ӗ�
		RET

_load_gdtr:		; void load_gdtr(int limit, int addr);
		MOV		AX,[ESP+4]		; limit
		MOV		[ESP+6],AX
		LGDT	[ESP+6]
		RET

_load_idtr:		; void load_idtr(int limit, int addr);
		MOV		AX,[ESP+4]		; limit
		MOV		[ESP+6],AX
		LIDT	[ESP+6]
		RET

_load_cr0:		; int load_cr0(void);
		MOV		EAX,CR0
		RET

_store_cr0:		; void store_cr0(int cr0);
		MOV		EAX,[ESP+4]
		MOV		CR0,EAX
		RET

_load_tr:		; void load_tr(int tr);
		LTR		[ESP+4]			; tr
		RET
		
; ========================================================================
;		   void disable_irq(int irq);
; ========================================================================
; Disable an interrupt request line by setting an 8259 bit.
; Equivalent code:
;	if(irq < 8){
;		out_byte(INT_M_CTLMASK, in_byte(INT_M_CTLMASK) | (1 << irq));
;	}
;	else{
;		out_byte(INT_S_CTLMASK, in_byte(INT_S_CTLMASK) | (1 << irq));
;	}
_disable_irq:
	mov	ecx, [esp + 4]		; irq
	pushf
	cli
	mov	ah, 1
	rol	ah, cl			; ah = (1 << (irq % 8))
	cmp	cl, 8
	jae	disable_8		; disable irq >= 8 at the slave 8259
disable_0:
	in	al, 0x21
	test	al, ah
	jnz	dis_already		; already disabled?
	or	al, ah
	out	0x21, al	; set bit at master 8259
	popf
	mov	eax, 1			; disabled by this function
	ret
disable_8:
	in	al, 0xA1
	test	al, ah
	jnz	dis_already		; already disabled?
	or	al, ah
	out	0xA1, al	; set bit at slave 8259
	popf
	mov	eax, 1			; disabled by this function
	ret
dis_already:
	popf
	xor	eax, eax		; already disabled
	ret

; ========================================================================
;		   void enable_irq(int irq);
; ========================================================================
; Enable an interrupt request line by clearing an 8259 bit.
; Equivalent code:
;	if(irq < 8){
;		out_byte(INT_M_CTLMASK, in_byte(INT_M_CTLMASK) & ~(1 << irq));
;	}
;	else{
;		out_byte(INT_S_CTLMASK, in_byte(INT_S_CTLMASK) & ~(1 << irq));
;	}
;
_enable_irq:
	mov	ecx, [esp + 4]		; irq
	pushf
	cli
	mov	ah, ~1
	rol	ah, cl			; ah = ~(1 << (irq % 8))
	cmp	cl, 8
	jae	enable_8		; enable irq >= 8 at the slave 8259
enable_0:
	in	al, 0x21
	and	al, ah
	out	0x21, al	; clear bit at master 8259
	popf
	ret
enable_8:
	in	al, 0xA1
	and	al, ah
	out	0xA1, al	; clear bit at slave 8259
	popf
	ret


; ------------------------------------------------------------------------
; void* memcpy(void* es:p_dst, void* ds:p_src, int size);
; ------------------------------------------------------------------------
_memcpy1:
	push	ebp
	mov	ebp, esp

	push	esi
	push	edi
	push	ecx

	mov	edi, [ebp + 8]	; Destination
	mov	esi, [ebp + 12]	; Source
	mov	ecx, [ebp + 16]	; Counter
.1:
	cmp	ecx, 0		; �жϼ�����
	jz	.2		; ������Ϊ��ʱ����

	mov	al, [ds:esi]		; ��
	inc	esi			; ��
					; �� ���ֽ��ƶ�
	mov	byte [es:edi], al	; ��
	inc	edi			; ��

	dec	ecx		; ��������һ
	jmp	.1		; ѭ��
.2:
	mov	eax, [ebp + 8]	; ����ֵ

	pop	ecx
	pop	edi
	pop	esi
	mov	esp, ebp
	pop	ebp

	ret			; ��������������
; memcpy ����-------------------------------------------------------------


; ------------------------------------------------------------------------
; void memset(void* p_dst, char ch, int size);
; ------------------------------------------------------------------------
_memset1:
	push	ebp
	mov	ebp, esp

	push	esi
	push	edi
	push	ecx

	mov	edi, [ebp + 8]	; Destination
	mov	edx, [ebp + 12]	; Char to be putted
	mov	ecx, [ebp + 16]	; Counter
.1:
	cmp	ecx, 0		; �жϼ�����
	jz	.2		; ������Ϊ��ʱ����

	mov	byte [edi], dl		; ��
	inc	edi			; ��

	dec	ecx		; ��������һ
	jmp	.1		; ѭ��
.2:

	pop	ecx
	pop	edi
	pop	esi
	mov	esp, ebp
	pop	ebp

	ret			; ��������������
; ------------------------------------------------------------------------
	
; ========================================================================
;                  void port_read(u16 port, void* buf, int n);
; ========================================================================
_port_read:
	;mov	edx, [esp + 4]		; port
	;mov	edi, [esp + 4 + 4]	; buf
	;mov	ecx, [esp + 4 + 4 + 4]	; n
	push edx
	push edi
	push ecx
	mov	edx, [esp + 4 + 12]		; port
	mov	edi, [esp + 4 + 4+ 12]	; buf
	mov	ecx, [esp + 4 + 4 + 4+ 12]	; n
	shr	ecx, 1
	cld
	rep	insw
	pop ecx
	pop edi
	pop edx
	ret
	
; ========================================================================
;                  void port_write(u16 port, void* buf, int n);
; ========================================================================
_port_write:
	push edx
	push esi
	push ecx
	mov	edx, [esp + 4 + 12]		; port
	mov	esi, [esp + 4 + 4 + 12]	; buf
	mov	ecx, [esp + 4 + 4 + 4 + 12]	; n
	shr	ecx, 1
	cld
	rep	outsw
	pop ecx
	pop esi
	pop edx
	ret



_asm_inthandler20:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler20
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler21:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler21
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD
		

_asm_inthandler2c:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler2c
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler0c:
		STI
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler0c
		CMP		EAX,0
		JNE		_asm_end_app
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		ADD		ESP,4			; INT 0x0c �ł��A���ꂪ�K�v
		IRETD
		
_asm_inthandler2e:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler2e
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler0d:
		STI
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler0d
		CMP		EAX,0			; ���������Ⴄ
		JNE		_asm_end_app	; ���������Ⴄ
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		ADD		ESP,4			; INT 0x0d �ł́A���ꂪ�K�v
		IRETD

_memtest_sub:	; unsigned int memtest_sub(unsigned int start, unsigned int end)
		PUSH	EDI						; �iEBX, ESI, EDI ���g�������̂Łj
		PUSH	ESI
		PUSH	EBX
		MOV		ESI,0xaa55aa55			; pat0 = 0xaa55aa55;
		MOV		EDI,0x55aa55aa			; pat1 = 0x55aa55aa;
		MOV		EAX,[ESP+12+4]			; i = start;
mts_loop:
		MOV		EBX,EAX
		ADD		EBX,0xffc				; p = i + 0xffc;
		MOV		EDX,[EBX]				; old = *p;
		MOV		[EBX],ESI				; *p = pat0;
		XOR		DWORD [EBX],0xffffffff	; *p ^= 0xffffffff;
		CMP		EDI,[EBX]				; if (*p != pat1) goto fin;
		JNE		mts_fin
		XOR		DWORD [EBX],0xffffffff	; *p ^= 0xffffffff;
		CMP		ESI,[EBX]				; if (*p != pat0) goto fin;
		JNE		mts_fin
		MOV		[EBX],EDX				; *p = old;
		ADD		EAX,0x1000				; i += 0x1000;
		CMP		EAX,[ESP+12+8]			; if (i <= end) goto mts_loop;
		JBE		mts_loop
		POP		EBX
		POP		ESI
		POP		EDI
		RET
mts_fin:
		MOV		[EBX],EDX				; *p = old;
		POP		EBX
		POP		ESI
		POP		EDI
		RET

_farjmp:		; void farjmp(int eip, int cs);
		JMP		FAR	[ESP+4]				; eip, cs
		RET

_farcall:		; void farcall(int eip, int cs);
		CALL	FAR	[ESP+4]				; eip, cs
		RET

_asm_hrb_api:
		STI
		PUSH	DS
		PUSH	ES
		PUSHAD		; ���ڱ���Ĵ���ֵ��PUSH
		PUSHAD		; ������hrb_api��ֵ��PUSH
		MOV		AX,SS
		MOV		DS,AX		; ������ϵͳ�öε�ַ����DS��ES
		MOV		ES,AX
		CALL	_hrb_api
		CMP		EAX,0		; ��EAX��Ϊ0ʱ�������
		JNE		_asm_end_app
		ADD		ESP,32
		POPAD
		POP		ES
		POP		DS
		IRETD
_asm_end_app:
;	EAX��tss.esp0�̔Ԓn
		MOV		ESP,[EAX]
		MOV		DWORD [EAX+4],0
		POPAD
		RET					; cmd_app�֋A��

_start_app:		; void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
		PUSHAD		; ��32λ�Ĵ�����ֵȫ����������
		MOV		EAX,[ESP+36]	; Ӧ�ó�����EIP
		MOV		ECX,[ESP+40]	; Ӧ�ó�����CS
		MOV		EDX,[ESP+44]	; Ӧ�ó�����ESP
		MOV		EBX,[ESP+48]	; Ӧ�ó�����DS/SS
		MOV		EBP,[ESP+52]	; tss.esp0�ĵ�ַ
		MOV		[EBP  ],ESP		; �������ϵͳ��ESP
		MOV		[EBP+4],SS		; �������ϵͳ��SS
		MOV		ES,BX
		MOV		DS,BX
		MOV		FS,BX
		MOV		GS,BX
;	�������ջ��������RETF��ת��Ӧ�ó���
		OR		ECX,3			; ��Ӧ�ó����öκź�3����OR����
		OR		EBX,3			; ��Ӧ�ó����öκź�3����OR����
		PUSH	EBX				; Ӧ�ó����SS
		PUSH	EDX				; Ӧ�ó����ESP
		PUSH	ECX				; Ӧ�ó����CS
		PUSH	EAX				; Ӧ�ó����EIP
		RETF
;	Ӧ�ó�������󲻻�ص�����


