BITS 32

		GLOBAL	_api_getmouse

[SECTION .text]

_api_getmouse:		; int api_getmouse(int mode, struct MOUSE_INFO *minfo);
		PUSH EBP
		MOV 	EAX, [ESP+8]
		MOV     EBP, [ESP+12]
		MOV		EDX, 28
		INT		0x40
		POP EBP
		RET
