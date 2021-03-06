; naskfunc
; TAB=4

[FORMAT "WCOFF"]				; オブジェクトファイルを作るモード	
[INSTRSET "i486p"]				; 486の命令まで使いたいという記述
[BITS 32]						; 32ビットモード用の機械語を作らせる
[FILE "naskfunc.nas"]			; ソースファイル名情報

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
		EXTERN	_hrb_api, _linux_api, _linux_api2, _do_page_fault, _add_print_flag

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
		PUSHFD		; PUSH EFLAGS という意味
		POP		EAX
		RET

_io_store_eflags:	; void io_store_eflags(int eflags);
		MOV		EAX,[ESP+4]
		PUSH	EAX
		POPFD		; POP EFLAGS という意味
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
	cmp	ecx, 0		; ﾅﾐｶﾏｼﾆﾊ�ﾆ�
	jz	.2		; ｼﾆﾊ�ﾆ�ﾎｪﾁ飜ｱﾌ�ｳ�

	mov	al, [ds:esi]		; ｩｷ
	inc	esi			; ｩｧ
					; ｩﾇ ﾖ�ﾗﾖｽﾚﾒﾆｶｯ
	mov	byte [es:edi], al	; ｩｧ
	inc	edi			; ｩｿ

	dec	ecx		; ｼﾆﾊ�ﾆ�ｼ�ﾒｻ
	jmp	.1		; ﾑｭｻｷ
.2:
	mov	eax, [ebp + 8]	; ｷｵｻﾘﾖｵ

	pop	ecx
	pop	edi
	pop	esi
	mov	esp, ebp
	pop	ebp

	ret			; ｺｯﾊ�ｽ睫�｣ｬｷｵｻﾘ
; memcpy ｽ睫�-------------------------------------------------------------


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
	cmp	ecx, 0		; ﾅﾐｶﾏｼﾆﾊ�ﾆ�
	jz	.2		; ｼﾆﾊ�ﾆ�ﾎｪﾁ飜ｱﾌ�ｳ�

	mov	byte [edi], dl		; ｩｷ
	inc	edi			; ｩｿ

	dec	ecx		; ｼﾆﾊ�ﾆ�ｼ�ﾒｻ
	jmp	.1		; ﾑｭｻｷ
.2:

	pop	ecx
	pop	edi
	pop	esi
	mov	esp, ebp
	pop	ebp

	ret			; ｺｯﾊ�ｽ睫�｣ｬｷｵｻﾘ
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
		PUSH	EAX      ;ESPﾗ�ﾎｪｲﾎﾊ�ｴｫｵﾝｸ�_inthandler20
		MOV		AX,SS    
		MOV		DS,AX    ;DS, ESﾉ靹ﾃﾎｪﾄﾚｺﾋｶﾎﾃ靆�ｷ�
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

_asm_inthandler0c:      ;Stack Exception
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
		ADD		ESP,4			; INT 0x0c でも、これが必要
		IRETD
		 
_asm_inthandler2e:     ;hd
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

_asm_inthandler0d:      ;General Protected Exception
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
		CMP		EAX,0			; ここだけ違う
		JNE		_asm_end_app	; ここだけ違う
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		ADD		ESP,4			; INT 0x0d では、これが必要
		IRETD
		
_asm_inthandler0e:
		;STI
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
		CALL   _do_page_fault
		ADD  ESP, 8
		POPAD
		POP  ES
		POP  DS
		POP  ECX
		IRETD


_memtest_sub:	; unsigned int memtest_sub(unsigned int start, unsigned int end)
		PUSH	EDI						; （EBX, ESI, EDI も使いたいので）
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
		PUSHAD		; ﾓﾃﾓﾚｱ｣ｴ貍ﾄｴ貳�ﾖｵｵﾃPUSH
		;ｽｫss,esp,eflags,cs,eipﾕ簟ｩﾗ�ﾎｪｲﾎﾊ�｣ｬｴｫｸ�hrb_api
		
		PUSH [ESP+48] ;SS
		PUSH [ESP+48] ;ESP
		PUSH [ESP+48] ;EFLAGS
		PUSH [ESP+48] ;CS
		PUSH [ESP+48] ;EIP
		
		PUSH	DS
		PUSH	ES
		PUSH    GS
		PUSH    FS
	
		PUSHAD		; ﾓﾃﾓﾚﾏ�hrb_apiｴｫﾖｵｵﾃPUSH
		
		
		MOV		AX,SS
		MOV		DS,AX		; ｽｫｲﾙﾗ�ﾏｵﾍｳﾓﾃｶﾎｵﾘﾖｷｴ貶�DSｺﾍES
		MOV		ES,AX
		CALL	_hrb_api
		CMP		EAX,0		; ｵｱEAXｲｻﾎｪ0ﾊｱｳﾌﾐ�ｽ睫�
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
;	EAXはtss.esp0の番地
		MOV		ESP,[EAX]  ;ESP = tss.esp0
		MOV		DWORD [EAX+4],0 ;tss.ss0 = 0
		POPAD
		RET					; cmd_appへ帰る
		
_asm_linux_api:
		STI
		PUSHAD		; ﾓﾃﾓﾚｱ｣ｴ貍ﾄｴ貳�ﾖｵｵﾃPUSH
		;ｽｫss,esp,eflags,cs,eipﾕ簟ｩﾗ�ﾎｪｲﾎﾊ�｣ｬｴｫｸ�hrb_api
		
		PUSH [ESP+48] ;SS
		PUSH [ESP+48] ;ESP
		PUSH [ESP+48] ;EFLAGS
		PUSH [ESP+48] ;CS
		PUSH [ESP+48] ;EIP
		
		
		PUSH	DS
		PUSH	ES
		PUSH    GS
		PUSH    FS
	
		
		PUSHAD		; ﾓﾃﾓﾚﾏ�hrb_apiｴｫﾖｵｵﾃPUSH
		;MOV FS, [ESP+48] ;ﾓﾃｻｧｿﾕｼ莊ﾄﾊ�ｾﾝｶﾎﾃ靆�ｷ�
		
		MOV		AX,SS
		MOV		DS,AX		; ｽｫｲﾙﾗ�ﾏｵﾍｳﾓﾃｶﾎｵﾘﾖｷｴ貶�DSｺﾍES
		MOV		ES,AX
		CALL	_linux_api2
		CMP		EAX,0		; ｵｱEAXｲｻﾎｪ0ﾊｱｳﾌﾐ�ｽ睫�
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
		PUSHAD		; ｽｫ32ﾎｻｼﾄｴ貳�ｵﾄﾖｵﾈｫｲｿｱ｣ｴ賺ﾂﾀｴ
		MOV		EAX,[ESP+36]	; ﾓｦﾓﾃｳﾌﾐ�ﾓﾃEIP
		MOV		ECX,[ESP+40]	; ﾓｦﾓﾃｳﾌﾐ�ﾓﾃCS
		MOV		EDX,[ESP+44]	; ﾓｦﾓﾃｳﾌﾐ�ﾓﾃESP
		MOV		EBX,[ESP+48]	; ﾓｦﾓﾃｳﾌﾐ�ﾓﾃDS/SS
		MOV		EBP,[ESP+52]	; tss.esp0ｵﾄｵﾘﾖｷ
		MOV		[EBP  ],ESP		; ｱ｣ｴ豐ﾙﾗ�ﾏｵﾍｳﾓﾃESP, ｱ｣ｴ贇ﾚtss.esp0ﾖﾐ
		MOV		[EBP+4],SS		; ｱ｣ｴ豐ﾙﾗ�ﾏｵﾍｳﾓﾃSS, ｱ｣ｴ贇ﾚtss.ss0ﾖﾐ
		MOV		ES,BX
		MOV		DS,BX
		MOV		FS,BX
		MOV		GS,BX
		;ﾏﾂﾃ豬�ﾕ�ﾕｻ｣ｬﾒﾑﾃ簽ﾃRETFﾌ�ﾗｪｵｽﾓｦﾓﾃｳﾌﾐ�｡｣RETFｽｫｻ眇ｫCSｺﾍEIPﾉ靹ﾃｳﾉﾓｦﾓﾃｳﾌﾐ�ｵﾄCSｺﾍEIP｡｣
		OR		ECX,3			; ｽｫﾓｦﾓﾃｳﾌﾐ�ﾓﾃｶﾎｺﾅｺﾍ3ｽ�ﾐﾐORﾔﾋﾋ�, ｽｫﾓｦﾓﾃｳﾌﾐ�ｵﾄｶﾎﾑ｡ﾔ�ﾗﾓ+3, ｼｴCS+3
		OR		EBX,3			; ｽｫﾓｦﾓﾃｳﾌﾐ�ﾓﾃｶﾎｺﾅｺﾍ3ｽ�ﾐﾐORﾔﾋﾋ�, ｽｫﾓｦﾓﾃｳﾌﾐ�ｵﾄｶﾎﾑ｡ﾔ�ﾗﾓ+3, ｼｴSS+3
		
		PUSH	EBX				; ﾓｦﾓﾃｳﾌﾐ�ｵﾄSS
		PUSH	EDX				; ﾓｦﾓﾃｳﾌﾐ�ｵﾄESP
		PUSH	ECX				; ﾓｦﾓﾃｳﾌﾐ�ｵﾄCS
		PUSH	EAX				; ﾓｦﾓﾃｳﾌﾐ�ｵﾄEIP
		MOV EDX, [ESP+56+16]       ; argc
		MOV EBX, [ESP+60+16]       ; argv
		RETF
;	ﾓｦﾓﾃｳﾌﾐ�ｽ睫�ｺ�ｲｻｻ盻ﾘｵｽﾕ簑�

_start_app_elf:		; void start_app_elf(int eip, int cs, int esp, int ds, int *tss_esp0, int argc, int argv);
		PUSHAD		; ｽｫ32ﾎｻｼﾄｴ貳�ｵﾄﾖｵﾈｫｲｿｱ｣ｴ賺ﾂﾀｴ
		MOV		EAX,[ESP+36]	; ﾓｦﾓﾃｳﾌﾐ�ﾓﾃEIP
		MOV		ECX,[ESP+40]	; ﾓｦﾓﾃｳﾌﾐ�ﾓﾃCS
		MOV		EDX,[ESP+44]	; ﾓｦﾓﾃｳﾌﾐ�ﾓﾃESP
		MOV		EBX,[ESP+48]	; ﾓｦﾓﾃｳﾌﾐ�ﾓﾃDS/SS
		MOV		EBP,[ESP+52]	; tss.esp0ｵﾄｵﾘﾖｷ
		MOV		[EBP  ],ESP		; ｱ｣ｴ豐ﾙﾗ�ﾏｵﾍｳﾓﾃESP, ｱ｣ｴ贇ﾚtss.esp0ﾖﾐ
		MOV		[EBP+4],SS		; ｱ｣ｴ豐ﾙﾗ�ﾏｵﾍｳﾓﾃSS, ｱ｣ｴ贇ﾚtss.ss0ﾖﾐ
		MOV		ES,BX
		MOV		DS,BX
		MOV		FS,BX
		MOV		GS,BX
;	ﾏﾂﾃ豬�ﾕ�ﾕｻ｣ｬﾒﾑﾃ簽ﾃRETFﾌ�ﾗｪｵｽﾓｦﾓﾃｳﾌﾐ�｡｣RETFｽｫｻ眇ｫCSｺﾍEIPﾉ靹ﾃｳﾉﾓｦﾓﾃｳﾌﾐ�ｵﾄCSｺﾍEIP｡｣ｵｫﾊﾇ｣ｬﾕｻﾊﾇﾈ郤ﾎｽ�ﾐﾐﾇﾐｻｻｵﾄ｣ｿ ﾎﾒｻｳﾒﾉﾊﾇｱ默�ﾆ�ﾉ�ｳﾉHRBｿﾉﾖｴﾐﾐﾎﾄｼ�ｵﾄﾊｱｺ�｣ｬｻ睫ﾗﾏﾈﾗ�ﾕ篋�SSｺﾍESPｵﾄﾉ靹ﾃ｡｣
		OR		ECX,3			; ｽｫﾓｦﾓﾃｳﾌﾐ�ﾓﾃｶﾎｺﾅｺﾍ3ｽ�ﾐﾐORﾔﾋﾋ�, ｽｫﾓｦﾓﾃｳﾌﾐ�ｵﾄｶﾎﾑ｡ﾔ�ﾗﾓ+3, ｼｴCS+3
		OR		EBX,3			; ｽｫﾓｦﾓﾃｳﾌﾐ�ﾓﾃｶﾎｺﾅｺﾍ3ｽ�ﾐﾐORﾔﾋﾋ�, ｽｫﾓｦﾓﾃｳﾌﾐ�ｵﾄｶﾎﾑ｡ﾔ�ﾗﾓ+3, ｼｴSS+3
		
		;MOV     SS, BX
		;MOV     ESP, EDX

		PUSH	EBX				; ﾓｦﾓﾃｳﾌﾐ�ｵﾄSS
		PUSH	EDX				; ﾓｦﾓﾃｳﾌﾐ�ｵﾄESP
		PUSH	ECX				; ﾓｦﾓﾃｳﾌﾐ�ｵﾄCS
		PUSH	EAX				; ﾓｦﾓﾃｳﾌﾐ�ｵﾄEIP
		MOV EDX, [ESP+56+16]       ; argc
		MOV EBX, [ESP+60+16]       ; argv
		
		RETF
;	ﾓｦﾓﾃｳﾌﾐ�ｽ睫�ｺ�ｲｻｻ盻ﾘｵｽﾕ簑�

_open_page:
	 CLI
	 mov eax,0x00400000                 ;PCD=PWT=0
     mov cr3,eax

     mov eax,cr0
     or eax,0x80000000
     mov cr0,eax                        ;ｿｪﾆ�ｷﾖﾒｳｻ�ﾖﾆ
	 STI
	 ret
	 
_get_cr3:
	mov eax, cr3
	ret

_get_cr0:
	mov eax, cr0
	ret

