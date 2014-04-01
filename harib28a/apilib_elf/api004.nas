BITS 32

		GLOBAL	_api_end

[SECTION .text]

_api_end:	; void api_end(void);
		MOV		EDX,4
		INT		0x40
