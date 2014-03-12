[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api033.nas"]

		GLOBAL	_api_filesize

[SECTION .text]
  
_api_filesize:		; int _api_filesize(const int fd); 
		MOV 	EAX, [ESP+4] ;fd 
		MOV		EDX, 33
		INT		0x40
		RET
