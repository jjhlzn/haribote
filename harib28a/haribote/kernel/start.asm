extern main
extern _api_end

bits 32

[section .text]

global _start

_start:
	;pop eax     ;esp
	;pop ecx     ;ss
	;mov esp, eax
	;mov ax, 15
	;mov ss, ax
	push ebx
	push edx
	call main
	
	push eax
	call _api_end
	
	hlt;
