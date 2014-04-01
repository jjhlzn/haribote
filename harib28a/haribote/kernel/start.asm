extern _main
extern _api_end

bits 32

[section .text]

global _start

_start:
	push eax
	push ecx
	call _main
	
	push eax
	call _api_end
	
	hlt;