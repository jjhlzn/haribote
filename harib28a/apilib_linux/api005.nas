BITS 32
		GLOBAL	open

[SECTION .text]
  
open:		;  int open(const char *pathname, int flags, int mode);
		PUSH EBX
		PUSH ECX
		MOV 	EBX, [ESP+12] ;pathname 
		MOV     ECX, [ESP+16] ;flags
		MOV     EDX, [ESP+20] ;mode
		MOV		EAX, 5
		INT		0x80
		POP ECX
		POP EBX
		RET
