BITS 32

		GLOBAL	_api_putstr0

[SECTION .text]

_api_putstr0:	; void api_putstr0(char *s);
		PUSH	EBX
		MOV		EDX,2
		MOV		EBX,[ESP+8]		; s
		INT		0x40
		POP		EBX
		RET
