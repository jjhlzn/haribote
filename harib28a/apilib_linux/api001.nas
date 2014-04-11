BITS 32

		GLOBAL	_api_end

[SECTION .text]

_api_end:	; void api_end(void);
		MOV		EAX,1
		INT		0x80
	
