; naskfunc
; TAB=4

[FORMAT "WCOFF"]				; 僆僽僕僃僋僩僼傽僀儖傪嶌傞儌乕僪	
[INSTRSET "i486p"]				; 486偺柦椷傑偱巊偄偨偄偲偄偆婰弎
[BITS 32]						; 32價僢僩儌乕僪梡偺婡夿岅傪嶌傜偣傞
[FILE "naskfunc.nas"]			; 僜乕僗僼傽僀儖柤忣曬

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
		GLOBAL	_asm_inthandler0d, _asm_end_app, _asm_inthandler0e
		GLOBAL	_memtest_sub
		GLOBAL	_farjmp, _farcall
		GLOBAL	_asm_hrb_api, _start_app, _asm_linux_api, _start_app_elf
		GLOBAL  _disable_irq, _enable_irq, _port_read,_port_write
		GLOBAL  _memcpy1, _memset1,_ud2
		GLOBAL  _open_page, _get_cr3, _get_cr0
		EXTERN	_inthandler20, _inthandler21
		EXTERN	_inthandler2c, _inthandler0d
		EXTERN 	_inthandler2e
		EXTERN	_inthandler0c
		EXTERN	_hrb_api, _linux_api, _linux_api2, _do_no_page

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
		PUSHFD		; PUSH EFLAGS 偲偄偆堄枴
		POP		EAX
		RET

_io_store_eflags:	; void io_store_eflags(int eflags);
		MOV		EAX,[ESP+4]
		PUSH	EAX
		POPFD		; POP EFLAGS 偲偄偆堄枴
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
; void* memcpy1(void* es:p_dst, void* ds:p_src, int size);
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
	cmp	ecx, 0		; 判断计数器
	jz	.2		; 计数器为零时跳出

	mov	al, [ds:esi]		; ┓
	inc	esi			; ┃
					; ┣ 逐字节移动
	mov	byte [es:edi], al	; ┃
	inc	edi			; ┛

	dec	ecx		; 计数器减一
	jmp	.1		; 循环
.2:
	mov	eax, [ebp + 8]	; 返回值

	pop	ecx
	pop	edi
	pop	esi
	mov	esp, ebp
	pop	ebp

	ret			; 函数结束，返回
; memcpy 结束-------------------------------------------------------------


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
	cmp	ecx, 0		; 判断计数器
	jz	.2		; 计数器为零时跳出

	mov	byte [edi], dl		; ┓
	inc	edi			; ┛

	dec	ecx		; 计数器减一
	jmp	.1		; 循环
.2:

	pop	ecx
	pop	edi
	pop	esi
	mov	esp, ebp
	pop	ebp

	ret			; 函数结束，返回
; ------------------------------------------------------------------------
	
; ========================================================================
;                  void port_read(u16 port, void* buf, int n);
; ========================================================================
_port_read:
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
		PUSH	EAX      ;ESP作为参数传递给_inthandler20
		MOV		AX,SS    
		MOV		DS,AX    ;DS, ES设置为内核段描述符
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
		ADD		ESP,4			; INT 0x0c 偱傕丄偙傟偑昁梫
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
		CMP		EAX,0			; 偙偙偩偗堘偆
		JNE		_asm_end_app	; 偙偙偩偗堘偆
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		ADD		ESP,4			; INT 0x0d 偱偼丄偙傟偑昁梫
		IRETD
		
_asm_inthandler0e:
		STI
		XCHG   [ESP], ECX
		PUSH   DS
		PUSH   ES
		PUSHAD
		MOV	 AX,SS
		MOV  DS, AX
		MOV  ES, AX
		MOV  EDX, CR2
		PUSH EDX
		PUSH ECX
		CALL   _do_no_page
		ADD  ESP, 8
		POPAD
		POP  ES
		POP  DS
		POP  ECX
		IRETD


_memtest_sub:	; unsigned int memtest_sub(unsigned int start, unsigned int end)
		PUSH	EDI						; 乮EBX, ESI, EDI 傕巊偄偨偄偺偱乯
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
		PUSHAD		; 用于保存寄存器值得PUSH
		;将ss,esp,eflags,cs,eip这些作为参数，传给hrb_api
		
		PUSH [ESP+48] ;SS
		PUSH [ESP+48] ;ESP
		PUSH [ESP+48] ;EFLAGS
		PUSH [ESP+48] ;CS
		PUSH [ESP+48] ;EIP
		
		PUSH	DS
		PUSH	ES
		PUSH    GS
		PUSH    FS
	
		PUSHAD		; 用于向hrb_api传值得PUSH
		
		
		MOV		AX,SS
		MOV		DS,AX		; 将操作系统用段地址存入DS和ES
		MOV		ES,AX
		CALL	_hrb_api
		CMP		EAX,0		; 当EAX不为0时程序结束
		JNE		_asm_end_app
		
		ADD		ESP,32
		
		POP     FS
		POP     GS
		POP		ES
		POP		DS
		
		ADD ESP,20 
		
		POPAD
		IRETD
_asm_end_app:
;	EAX偼tss.esp0偺斣抧
		MOV		ESP,[EAX]  ;ESP = tss.esp0
		MOV		DWORD [EAX+4],0 ;tss.ss0 = 0
		POPAD
		RET					; cmd_app傊婣傞
		
_asm_linux_api:
		STI
		PUSHAD		; 用于保存寄存器值得PUSH
		;将ss,esp,eflags,cs,eip这些作为参数，传给hrb_api
		
		PUSH [ESP+48] ;SS
		PUSH [ESP+48] ;ESP
		PUSH [ESP+48] ;EFLAGS
		PUSH [ESP+48] ;CS
		PUSH [ESP+48] ;EIP
		
		
		PUSH	DS
		PUSH	ES
		PUSH    GS
		PUSH    FS
	
		
		PUSHAD		; 用于向hrb_api传值得PUSH
		;MOV FS, [ESP+48] ;用户空间的数据段描述符
		
		MOV		AX,SS
		MOV		DS,AX		; 将操作系统用段地址存入DS和ES
		MOV		ES,AX
		CALL	_linux_api2
		CMP		EAX,0		; 当EAX不为0时程序结束
		JNE		_asm_end_app
		
		ADD		ESP,32
		
		POP     FS
		POP     GS
		POP		ES
		POP		DS
		
		ADD ESP,20 
		
		POPAD
		IRETD
		  


_start_app:		; void start_app(int eip, int cs, int esp, int ds, int *tss_esp0, int argc, int argv);
		PUSHAD		; 将32位寄存器的值全部保存下来
		MOV		EAX,[ESP+36]	; 应用程序用EIP
		MOV		ECX,[ESP+40]	; 应用程序用CS
		MOV		EDX,[ESP+44]	; 应用程序用ESP
		MOV		EBX,[ESP+48]	; 应用程序用DS/SS
		MOV		EBP,[ESP+52]	; tss.esp0的地址
		MOV		[EBP  ],ESP		; 保存操作系统用ESP, 保存在tss.esp0中
		MOV		[EBP+4],SS		; 保存操作系统用SS, 保存在tss.ss0中
		MOV		ES,BX
		MOV		DS,BX
		MOV		FS,BX
		MOV		GS,BX
;	下面调整栈，已免用RETF跳转到应用程序。RETF将会将CS和EIP设置成应用程序的CS和EIP。但是，栈是如何进行切换的？ 我怀疑是编译器生成HRB可执行文件的时候，会首先做这个SS和ESP的设置。
		OR		ECX,3			; 将应用程序用段号和3进行OR运算, 将应用程序的段选择子+3, 即CS+3
		OR		EBX,3			; 将应用程序用段号和3进行OR运算, 将应用程序的段选择子+3, 即SS+3
		
		PUSH	EBX				; 应用程序的SS
		PUSH	EDX				; 应用程序的ESP
		PUSH	ECX				; 应用程序的CS
		PUSH	EAX				; 应用程序的EIP
		MOV EDX, [ESP+56+16]       ; argc
		MOV EBX, [ESP+60+16]       ; argv
		RETF
;	应用程序结束后不会回到这里

_start_app_elf:		; void start_app_elf(int eip, int cs, int esp, int ds, int *tss_esp0, int argc, int argv);
		PUSHAD		; 将32位寄存器的值全部保存下来
		MOV		EAX,[ESP+36]	; 应用程序用EIP
		MOV		ECX,[ESP+40]	; 应用程序用CS
		MOV		EDX,[ESP+44]	; 应用程序用ESP
		MOV		EBX,[ESP+48]	; 应用程序用DS/SS
		MOV		EBP,[ESP+52]	; tss.esp0的地址
		MOV		[EBP  ],ESP		; 保存操作系统用ESP, 保存在tss.esp0中
		MOV		[EBP+4],SS		; 保存操作系统用SS, 保存在tss.ss0中
		MOV		ES,BX
		MOV		DS,BX
		MOV		FS,BX
		MOV		GS,BX
;	下面调整栈，已免用RETF跳转到应用程序。RETF将会将CS和EIP设置成应用程序的CS和EIP。但是，栈是如何进行切换的？ 我怀疑是编译器生成HRB可执行文件的时候，会首先做这个SS和ESP的设置。
		OR		ECX,3			; 将应用程序用段号和3进行OR运算, 将应用程序的段选择子+3, 即CS+3
		OR		EBX,3			; 将应用程序用段号和3进行OR运算, 将应用程序的段选择子+3, 即SS+3
		
		;MOV     SS, BX
		;MOV     ESP, EDX

		PUSH	EBX				; 应用程序的SS
		PUSH	EDX				; 应用程序的ESP
		PUSH	ECX				; 应用程序的CS
		PUSH	EAX				; 应用程序的EIP
		;MOV EDX, [ESP+56+16]       ; argc
		;MOV EBX, [ESP+60+16]       ; argv
		RETF
;	应用程序结束后不会回到这里

_open_page:
	 CLI
	 mov eax,0x00400000                 ;PCD=PWT=0
     mov cr3,eax

     mov eax,cr0
     or eax,0x80000000
     mov cr0,eax                        ;开启分页机制
	 STI
	 ret
	 
_get_cr3:
	mov eax, cr3
	ret

_get_cr0:
	mov eax, cr0
	ret

