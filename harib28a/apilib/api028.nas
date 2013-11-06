[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api028.nas"]

		GLOBAL	_api_getmouse

[SECTION .text]

_api_getmouse:		; int api_getmouse(int mode,struct MOUSE_INFO *minfo);
		MOV 	EAX, [ESP+4]
		MOV		EBX, [ESP+8]
		MOV     EBP, [ESP+12]
		MOV		EDX, 28
		INT		0x40
		RET
