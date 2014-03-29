[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[OPTIMIZE 1]
[OPTION 1]
[BITS 32]
	EXTERN	_init_gdtidt
	EXTERN	_init_pic
	EXTERN	_io_sti
	EXTERN	_fifo32_init
	EXTERN	_init_pit
	EXTERN	_init_keyboard
	EXTERN	_enable_mouse
	EXTERN	_io_out8
	EXTERN	_memtest
	EXTERN	_memman_init
	EXTERN	_memman_free
	EXTERN	_init_palette
	EXTERN	_shtctl_init
	EXTERN	_task_init
	EXTERN	_task_run
	EXTERN	_sheet_alloc
	EXTERN	_memman_alloc_4k
	EXTERN	_sheet_setbuf
	EXTERN	_init_screen8
	EXTERN	_log_win
	EXTERN	_init_mouse_cursor8
	EXTERN	_sheet_slide
	EXTERN	_sheet_updown
	EXTERN	_memman_total
	EXTERN	_fifo32_put
	EXTERN	_file_readfat
	EXTERN	_init_hd
	EXTERN	_init_fs
	EXTERN	_file_search
	EXTERN	_file_loadfile2
	EXTERN	_memman_free_4k
	EXTERN	_fifo32_status
	EXTERN	_io_cli
	EXTERN	_fifo32_get
	EXTERN	_sheet_free
	EXTERN	_taskctl
	EXTERN	_mouse_decode
	EXTERN	_asm_end_app
	EXTERN	_keytable0.0
	EXTERN	_wait_KBC_sendready
	EXTERN	_cons_putstr0
	EXTERN	_keytable1.1
	EXTERN	_task_sleep
	EXTERN	_hankaku
	EXTERN	_change_wtitle8
	EXTERN	_task_alloc
	EXTERN	_console_task
	EXTERN	_log_task
	EXTERN	_make_window8
	EXTERN	_make_textbox8
	EXTERN	__alloca
	EXTERN	_sprintf
	EXTERN	_boxfill8
	EXTERN	_putfonts8_asc
	EXTERN	_info_BMP
	EXTERN	_info_JPEG
	EXTERN	_decode0_JPEG
	EXTERN	_decode0_BMP
	EXTERN	_bootinfo
	EXTERN	_vsprintf
	EXTERN	_ud2
[FILE "bootpack.c"]
[SECTION .data]
	ALIGNB	4
_bootinfo:
	DD	4080
	ALIGNB	4
_log_win:
	DD	0
_keytable0.0:
	DB	0
	DB	0
	DB	49
	DB	50
	DB	51
	DB	52
	DB	53
	DB	54
	DB	55
	DB	56
	DB	57
	DB	48
	DB	45
	DB	94
	DB	8
	DB	0
	DB	81
	DB	87
	DB	69
	DB	82
	DB	84
	DB	89
	DB	85
	DB	73
	DB	79
	DB	80
	DB	64
	DB	91
	DB	10
	DB	0
	DB	65
	DB	83
	DB	68
	DB	70
	DB	71
	DB	72
	DB	74
	DB	75
	DB	76
	DB	59
	DB	58
	DB	0
	DB	0
	DB	93
	DB	90
	DB	88
	DB	67
	DB	86
	DB	66
	DB	78
	DB	77
	DB	44
	DB	46
	DB	47
	DB	0
	DB	42
	DB	0
	DB	32
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	55
	DB	56
	DB	57
	DB	45
	DB	52
	DB	53
	DB	54
	DB	43
	DB	49
	DB	50
	DB	51
	DB	48
	DB	46
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	92
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	92
	DB	0
	DB	0
_keytable1.1:
	DB	0
	DB	0
	DB	33
	DB	64
	DB	35
	DB	36
	DB	37
	DB	94
	DB	38
	DB	42
	DB	40
	DB	41
	DB	95
	DB	43
	DB	8
	DB	0
	DB	81
	DB	87
	DB	69
	DB	82
	DB	84
	DB	89
	DB	85
	DB	73
	DB	79
	DB	80
	DB	96
	DB	123
	DB	10
	DB	0
	DB	65
	DB	83
	DB	68
	DB	70
	DB	71
	DB	72
	DB	74
	DB	75
	DB	76
	DB	43
	DB	42
	DB	0
	DB	0
	DB	125
	DB	90
	DB	88
	DB	67
	DB	86
	DB	66
	DB	78
	DB	77
	DB	60
	DB	62
	DB	63
	DB	0
	DB	42
	DB	0
	DB	32
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	55
	DB	56
	DB	57
	DB	45
	DB	52
	DB	53
	DB	54
	DB	43
	DB	49
	DB	50
	DB	51
	DB	48
	DB	46
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	95
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	0
	DB	124
	DB	0
	DB	0
LC0:
	DB	"task_a",0x00
LC1:
	DB	"memory %dMB free : %dKB",0x00
LC2:
	DB	"dd1 = %8x, dd2 = %8x, dd3 = %8x, dd4 = %8x",0x00
LC3:
	DB	"cyl = %u, head = %u, wpcom = %u ctl = %u, lzone = %u, sect = %u",0x00
LC4:
	DB	"nihongo.fnt",0x00
LC5:
	DB	0x0A,"Break(key) :",0x0A,0x00
[SECTION .text]
	GLOBAL	_HariMain
_HariMain:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EDI
	PUSH	ESI
	MOV	ESI,LC0
	PUSH	EBX
	SUB	ESP,1116
	MOV	DWORD [-1056+EBP],-1
	MOV	DWORD [-1060+EBP],0
	MOV	DWORD [-1064+EBP],2147483647
	MOV	DWORD [-1068+EBP],0
	MOV	DWORD [-1084+EBP],0
	MOV	AL,BYTE [4081]
	SAR	AL,4
	MOV	EDX,EAX
	AND	EDX,7
	MOV	DWORD [-1088+EBP],EDX
	MOV	DWORD [-1092+EBP],-1
	MOV	DWORD [-1096+EBP],-1
	MOV	DWORD [-1100+EBP],-1
	MOV	DWORD [-1104+EBP],0
	MOV	DWORD [-1108+EBP],0
	CALL	_init_gdtidt
	CALL	_init_pic
	CALL	_io_sti
	LEA	EAX,DWORD [-636+EBP]
	PUSH	0
	PUSH	EAX
	LEA	EAX,DWORD [-92+EBP]
	PUSH	128
	PUSH	EAX
	CALL	_fifo32_init
	LEA	EDX,DWORD [-92+EBP]
	MOV	DWORD [4076],EDX
	CALL	_init_pit
	PUSH	256
	LEA	EAX,DWORD [-92+EBP]
	PUSH	EAX
	CALL	_init_keyboard
	LEA	EDX,DWORD [-92+EBP]
	LEA	EAX,DWORD [-780+EBP]
	PUSH	EAX
	PUSH	512
	PUSH	EDX
	CALL	_enable_mouse
	ADD	ESP,36
	PUSH	248
	PUSH	33
	CALL	_io_out8
	PUSH	175
	PUSH	161
	CALL	_io_out8
	LEA	EAX,DWORD [-764+EBP]
	PUSH	0
	PUSH	EAX
	LEA	EAX,DWORD [-124+EBP]
	PUSH	32
	PUSH	EAX
	CALL	_fifo32_init
	ADD	ESP,32
	PUSH	-1073741825
	PUSH	4194304
	CALL	_memtest
	PUSH	3932160
	MOV	DWORD [-1072+EBP],EAX
	CALL	_memman_init
	PUSH	647168
	PUSH	4096
	PUSH	3932160
	CALL	_memman_free
	MOV	EAX,DWORD [-1072+EBP]
	SUB	EAX,4194304
	PUSH	EAX
	PUSH	4194304
	PUSH	3932160
	CALL	_memman_free
	ADD	ESP,36
	CALL	_init_palette
	MOVSX	EAX,WORD [4086]
	PUSH	EAX
	MOVSX	EAX,WORD [4084]
	PUSH	EAX
	PUSH	DWORD [4088]
	PUSH	3932160
	CALL	_shtctl_init
	PUSH	3932160
	MOV	DWORD [-1044+EBP],EAX
	CALL	_task_init
	MOV	EDI,EAX
	MOV	ECX,7
	CLD
	ADD	EDI,12
	MOV	DWORD [-1080+EBP],EAX
	MOV	DWORD [-68+EBP],EAX
	REP
	MOVSB
	PUSH	2
	PUSH	1
	PUSH	DWORD [-1080+EBP]
	CALL	_task_run
	ADD	ESP,32
	MOV	EAX,DWORD [-1080+EBP]
	MOV	EDX,DWORD [-1044+EBP]
	MOV	DWORD [4068],EDX
	MOV	BYTE [236+EAX],0
	PUSH	EDX
	CALL	_sheet_alloc
	MOV	ESI,EAX
	MOVSX	EDX,WORD [4086]
	MOVSX	EAX,WORD [4084]
	IMUL	EAX,EDX
	PUSH	EAX
	PUSH	3932160
	CALL	_memman_alloc_4k
	PUSH	-1
	MOV	EBX,EAX
	MOVSX	EAX,WORD [4086]
	PUSH	EAX
	MOVSX	EAX,WORD [4084]
	PUSH	EAX
	PUSH	EBX
	PUSH	ESI
	CALL	_sheet_setbuf
	ADD	ESP,32
	MOVSX	EAX,WORD [4086]
	PUSH	EAX
	MOVSX	EAX,WORD [4084]
	PUSH	EAX
	PUSH	EBX
	LEA	EBX,DWORD [-1036+EBP]
	CALL	_init_screen8
	PUSH	DWORD [-1072+EBP]
	PUSH	DWORD [-1044+EBP]
	CALL	_open_console
	PUSH	DWORD [-1072+EBP]
	PUSH	DWORD [-1044+EBP]
	MOV	DWORD [-1112+EBP],EAX
	CALL	_open_log_console
	PUSH	DWORD [-1044+EBP]
	MOV	DWORD [_log_win],EAX
	CALL	_sheet_alloc
	ADD	ESP,32
	MOV	DWORD [-1076+EBP],EAX
	PUSH	99
	PUSH	16
	PUSH	16
	PUSH	EBX
	PUSH	EAX
	CALL	_sheet_setbuf
	PUSH	99
	PUSH	EBX
	MOV	EBX,2
	CALL	_init_mouse_cursor8
	MOVSX	EAX,WORD [4084]
	LEA	ECX,DWORD [-16+EAX]
	MOV	EAX,ECX
	CDQ
	IDIV	EBX
	MOV	DWORD [-1048+EBP],EAX
	MOVSX	EAX,WORD [4086]
	PUSH	0
	LEA	ECX,DWORD [-44+EAX]
	PUSH	0
	MOV	EAX,ECX
	PUSH	ESI
	CDQ
	IDIV	EBX
	MOV	DWORD [-1052+EBP],EAX
	CALL	_sheet_slide
	ADD	ESP,40
	PUSH	4
	PUSH	20
	PUSH	DWORD [-1112+EBP]
	CALL	_sheet_slide
	PUSH	4
	PUSH	520
	PUSH	DWORD [_log_win]
	CALL	_sheet_slide
	PUSH	DWORD [-1052+EBP]
	PUSH	DWORD [-1048+EBP]
	PUSH	DWORD [-1076+EBP]
	CALL	_sheet_slide
	ADD	ESP,36
	PUSH	0
	PUSH	ESI
	CALL	_sheet_updown
	PUSH	1
	PUSH	DWORD [_log_win]
	CALL	_sheet_updown
	PUSH	2
	PUSH	DWORD [-1112+EBP]
	CALL	_sheet_updown
	PUSH	3
	PUSH	DWORD [-1076+EBP]
	CALL	_sheet_updown
	ADD	ESP,32
	PUSH	DWORD [-1112+EBP]
	CALL	_keywin_on
	MOVZX	EDX,WORD [3840]
	MOV	DWORD [-1120+EBP],EDX
	MOV	AL,BYTE [3842]
	MOV	BYTE [-1121+EBP],AL
	MOV	DL,BYTE [3848]
	MOV	BYTE [-1122+EBP],DL
	MOVZX	EDI,WORD [3845]
	MOVZX	ESI,WORD [3852]
	MOV	BL,BYTE [3854]
	PUSH	3932160
	CALL	_memman_total
	SHR	EAX,10
	MOV	DWORD [ESP],EAX
	MOV	EAX,DWORD [-1072+EBP]
	SHR	EAX,20
	PUSH	EAX
	PUSH	LC1
	CALL	_debug
	PUSH	DWORD [3852]
	PUSH	DWORD [3848]
	PUSH	DWORD [3844]
	PUSH	DWORD [3840]
	PUSH	LC2
	CALL	_debug
	ADD	ESP,36
	MOVZX	EBX,BL
	PUSH	EBX
	PUSH	ESI
	MOVZX	EAX,BYTE [-1122+EBP]
	PUSH	EAX
	PUSH	EDI
	MOVZX	EAX,BYTE [-1121+EBP]
	PUSH	EAX
	MOV	EAX,DWORD [-1120+EBP]
	PUSH	EAX
	PUSH	LC3
	CALL	_debug
	LEA	EAX,DWORD [-124+EBP]
	PUSH	237
	PUSH	EAX
	CALL	_fifo32_put
	LEA	EDX,DWORD [-124+EBP]
	ADD	ESP,36
	PUSH	DWORD [-1088+EBP]
	PUSH	EDX
	CALL	_fifo32_put
	PUSH	11520
	PUSH	3932160
	CALL	_memman_alloc_4k
	PUSH	1049088
	MOV	EBX,EAX
	PUSH	EAX
	CALL	_file_readfat
	LEA	EAX,DWORD [-92+EBP]
	PUSH	EAX
	CALL	_init_hd
	CALL	_init_fs
	PUSH	224
	PUSH	1058304
	PUSH	LC4
	CALL	_file_search
	ADD	ESP,40
	MOV	EDX,EAX
	TEST	EAX,EAX
	JE	L2
	MOV	EAX,DWORD [28+EAX]
	PUSH	EBX
	MOV	DWORD [-1040+EBP],EAX
	LEA	EAX,DWORD [-1040+EBP]
	PUSH	EAX
	MOVZX	EAX,WORD [26+EDX]
	PUSH	EAX
	CALL	_file_loadfile2
	ADD	ESP,12
	MOV	ECX,EAX
L3:
	MOV	DWORD [4072],ECX
	PUSH	11520
	PUSH	EBX
	PUSH	3932160
	CALL	_memman_free_4k
L100:
	ADD	ESP,12
L14:
	LEA	EBX,DWORD [-124+EBP]
	PUSH	EBX
	CALL	_fifo32_status
	POP	ECX
	TEST	EAX,EAX
	JLE	L17
	CMP	DWORD [-1092+EBP],0
	JS	L104
L17:
	LEA	EBX,DWORD [-92+EBP]
	CALL	_io_cli
	PUSH	EBX
	CALL	_fifo32_status
	POP	EDX
	TEST	EAX,EAX
	JE	L105
	PUSH	EBX
	CALL	_fifo32_get
	MOV	DWORD [-1040+EBP],EAX
	CALL	_io_sti
	POP	EAX
	CMP	DWORD [-1112+EBP],0
	JE	L24
	MOV	EDX,DWORD [-1112+EBP]
	CMP	DWORD [28+EDX],0
	JNE	L24
	MOV	EDX,DWORD [-1044+EBP]
	MOV	DWORD [-1112+EBP],0
	MOV	EAX,DWORD [16+EDX]
	CMP	EAX,1
	JE	L24
	MOV	EDX,DWORD [-1044+EBP]
	MOV	EAX,DWORD [16+EDX+EAX*4]
	PUSH	EAX
	MOV	DWORD [-1112+EBP],EAX
	CALL	_keywin_on
	POP	EAX
L24:
	MOV	EDX,DWORD [-1040+EBP]
	LEA	EAX,DWORD [-256+EDX]
	CMP	EAX,255
	JBE	L106
	LEA	EAX,DWORD [-512+EDX]
	CMP	EAX,255
	JBE	L107
	LEA	EAX,DWORD [-768+EDX]
	CMP	EAX,255
	JBE	L108
	LEA	EAX,DWORD [-1024+EDX]
	CMP	EAX,999
	JBE	L109
	LEA	EAX,DWORD [-2024+EDX]
	CMP	EAX,255
	JA	L14
	IMUL	EAX,EDX,40
	ADD	EAX,DWORD [-1044+EBP]
	PUSH	42240
	PUSH	DWORD [-79916+EAX]
	PUSH	3932160
	LEA	EBX,DWORD [-79916+EAX]
	CALL	_memman_free_4k
	PUSH	EBX
	CALL	_sheet_free
	ADD	ESP,16
	JMP	L14
L109:
	IMUL	EDX,EDX,496
	ADD	EDX,DWORD [_taskctl]
	LEA	EAX,DWORD [-503816+EDX]
	PUSH	EAX
	CALL	_close_constask
L99:
	POP	EAX
	JMP	L14
L108:
	IMUL	EAX,EDX,40
	MOV	EDX,DWORD [-1044+EBP]
	LEA	EAX,DWORD [-29676+EAX+EDX*1]
	PUSH	EAX
	CALL	_close_console
	JMP	L99
L107:
	MOVZX	EAX,BYTE [-1040+EBP]
	PUSH	EAX
	LEA	EAX,DWORD [-780+EBP]
	PUSH	EAX
	CALL	_mouse_decode
	POP	ECX
	POP	EBX
	TEST	EAX,EAX
	JE	L14
	MOV	EAX,DWORD [-772+EBP]
	MOV	EDX,DWORD [-776+EBP]
	ADD	DWORD [-1052+EBP],EAX
	ADD	DWORD [-1048+EBP],EDX
	JS	L110
L56:
	CMP	DWORD [-1052+EBP],0
	JS	L111
L57:
	MOVSX	EAX,WORD [4084]
	DEC	EAX
	CMP	DWORD [-1048+EBP],EAX
	JLE	L58
	MOV	DWORD [-1048+EBP],EAX
L58:
	MOVSX	EAX,WORD [4086]
	DEC	EAX
	CMP	DWORD [-1052+EBP],EAX
	JLE	L59
	MOV	DWORD [-1052+EBP],EAX
L59:
	MOV	EAX,DWORD [-1048+EBP]
	MOV	EDX,DWORD [-1052+EBP]
	MOV	DWORD [-1056+EBP],EAX
	MOV	DWORD [-1060+EBP],EDX
	TEST	DWORD [-768+EBP],1
	JE	L60
	CMP	DWORD [-1096+EBP],0
	JS	L112
	MOV	ESI,DWORD [-1048+EBP]
	MOV	EAX,DWORD [-1104+EBP]
	SUB	ESI,DWORD [-1096+EBP]
	MOV	EDI,DWORD [-1052+EBP]
	SUB	EDI,DWORD [-1100+EBP]
	MOV	EDX,DWORD [-1052+EBP]
	LEA	ESI,DWORD [2+ESI+EAX*1]
	MOV	DWORD [-1100+EBP],EDX
	MOV	DWORD [-1064+EBP],ESI
	ADD	DWORD [-1068+EBP],EDI
	AND	DWORD [-1064+EBP],-4
L75:
	MOV	EDX,DWORD [-1044+EBP]
	MOV	EBX,DWORD [16+EDX]
L103:
	DEC	EBX
	TEST	EBX,EBX
	JLE	L14
	MOV	EAX,DWORD [-1044+EBP]
	MOV	EDI,DWORD [-1052+EBP]
	MOV	ESI,DWORD [-1048+EBP]
	MOV	EDX,DWORD [20+EAX+EBX*4]
	SUB	EDI,DWORD [16+EDX]
	SUB	ESI,DWORD [12+EDX]
	JS	L103
	CMP	ESI,DWORD [4+EDX]
	JGE	L103
	TEST	EDI,EDI
	JS	L103
	CMP	EDI,DWORD [8+EDX]
	JGE	L103
	MOV	EAX,DWORD [36+EDX]
	TEST	EAX,EAX
	JE	L103
	CMP	EAX,DWORD [-1080+EBP]
	JE	L103
	CMP	DWORD [80+EAX],0
	JE	L103
	SAL	ESI,17
	LEA	EAX,DWORD [ESI+EDI*8]
	ADD	EAX,DWORD [-768+EBP]
	SUB	EAX,-2147483648
	PUSH	EAX
	MOV	EAX,DWORD [36+EDX]
	ADD	EAX,52
	PUSH	EAX
	CALL	_fifo32_put
	POP	EDX
	POP	ECX
	JMP	L103
L112:
	MOV	EAX,DWORD [-1044+EBP]
	MOV	ECX,DWORD [16+EAX]
	LEA	EBX,DWORD [-1+ECX]
	TEST	EBX,EBX
	JLE	L75
L73:
	MOV	EDX,DWORD [-1044+EBP]
	MOV	EDI,DWORD [-1052+EBP]
	MOV	ESI,DWORD [-1048+EBP]
	MOV	EDX,DWORD [20+EDX+EBX*4]
	MOV	DWORD [-1108+EBP],EDX
	SUB	EDI,DWORD [16+EDX]
	SUB	ESI,DWORD [12+EDX]
	JS	L64
	MOV	EAX,DWORD [4+EDX]
	CMP	ESI,EAX
	JGE	L64
	TEST	EDI,EDI
	JS	L64
	CMP	EDI,DWORD [8+EDX]
	JGE	L64
	IMUL	EAX,EDI
	MOV	DWORD [-1128+EBP],EAX
	MOV	EDX,DWORD [EDX]
	MOV	EAX,DWORD [-1128+EBP]
	ADD	EAX,ESI
	MOVZX	EAX,BYTE [EAX+EDX*1]
	MOV	EDX,DWORD [-1108+EBP]
	CMP	EAX,DWORD [20+EDX]
	JNE	L113
L64:
	DEC	EBX
	TEST	EBX,EBX
	JG	L73
	JMP	L75
L113:
	LEA	EAX,DWORD [-1+ECX]
	PUSH	EAX
	PUSH	EDX
	CALL	_sheet_updown
	POP	EAX
	MOV	EAX,DWORD [-1112+EBP]
	POP	EDX
	CMP	DWORD [-1108+EBP],EAX
	JE	L68
	PUSH	EAX
	CALL	_keywin_off
	MOV	EDX,DWORD [-1108+EBP]
	PUSH	EDX
	MOV	DWORD [-1112+EBP],EDX
	CALL	_keywin_on
	POP	EBX
	POP	EAX
L68:
	CMP	ESI,2
	JLE	L69
	MOV	EDX,DWORD [-1108+EBP]
	MOV	EAX,DWORD [4+EDX]
	SUB	EAX,3
	CMP	ESI,EAX
	JGE	L69
	CMP	EDI,2
	JLE	L69
	CMP	EDI,20
	JG	L69
	MOV	EAX,DWORD [-1048+EBP]
	MOV	EDX,DWORD [-1052+EBP]
	MOV	DWORD [-1096+EBP],EAX
	MOV	DWORD [-1100+EBP],EDX
	MOV	EAX,DWORD [-1108+EBP]
	MOV	EDX,DWORD [-1108+EBP]
	MOV	EAX,DWORD [12+EAX]
	MOV	EDX,DWORD [16+EDX]
	MOV	DWORD [-1104+EBP],EAX
	MOV	DWORD [-1068+EBP],EDX
L69:
	MOV	EAX,DWORD [-1108+EBP]
	MOV	EDX,DWORD [4+EAX]
	LEA	EAX,DWORD [-21+EDX]
	CMP	EAX,ESI
	JG	L75
	LEA	EAX,DWORD [-5+EDX]
	CMP	ESI,EAX
	JGE	L75
	CMP	EDI,4
	JLE	L75
	CMP	EDI,18
	JG	L75
	MOV	EDX,DWORD [-1108+EBP]
	TEST	BYTE [28+EDX],16
	JNE	L98
	MOV	EBX,DWORD [36+EDX]
	PUSH	-1
	PUSH	EDX
	CALL	_sheet_updown
	PUSH	DWORD [-1112+EBP]
	CALL	_keywin_off
	MOV	EDX,DWORD [-1044+EBP]
	MOV	EAX,DWORD [16+EDX]
	MOV	EAX,DWORD [16+EDX+EAX*4]
	PUSH	EAX
	MOV	DWORD [-1112+EBP],EAX
	CALL	_keywin_on
	CALL	_io_cli
	LEA	EAX,DWORD [52+EBX]
	PUSH	4
	PUSH	EAX
	CALL	_fifo32_put
	CALL	_io_sti
	ADD	ESP,24
	JMP	L75
L98:
	MOV	EAX,DWORD [-1108+EBP]
	MOV	EBX,DWORD [36+EAX]
	CALL	_io_cli
	LEA	EAX,DWORD [88+EBX]
	MOV	DWORD [124+EBX],EAX
	MOV	DWORD [116+EBX],_asm_end_app
	CALL	_io_sti
	PUSH	0
	PUSH	-1
	PUSH	EBX
	CALL	_task_run
L102:
	ADD	ESP,12
	JMP	L75
L60:
	MOV	DWORD [-1096+EBP],-1
	CMP	DWORD [-1064+EBP],2147483647
	JE	L75
	PUSH	DWORD [-1068+EBP]
	PUSH	DWORD [-1064+EBP]
	PUSH	DWORD [-1108+EBP]
	CALL	_sheet_slide
	MOV	DWORD [-1064+EBP],2147483647
	JMP	L102
L111:
	MOV	DWORD [-1052+EBP],0
	JMP	L57
L110:
	MOV	DWORD [-1048+EBP],0
	JMP	L56
L106:
	CMP	EDX,383
	JG	L28
	CMP	DWORD [-1084+EBP],0
	JNE	L29
	MOV	AL,BYTE [_keytable0.0-256+EDX]
L101:
	MOV	BYTE [-60+EBP],AL
L31:
	MOV	DL,BYTE [-60+EBP]
	LEA	EAX,DWORD [-65+EDX]
	CMP	AL,25
	JA	L32
	TEST	DWORD [-1088+EBP],4
	JNE	L97
	CMP	DWORD [-1084+EBP],0
	JE	L34
L32:
	MOV	AL,BYTE [-60+EBP]
	TEST	AL,AL
	JE	L36
	CMP	DWORD [-1112+EBP],0
	JE	L36
	MOVSX	EAX,AL
	ADD	EAX,256
	MOV	EDX,DWORD [-1112+EBP]
	PUSH	EAX
	MOV	EAX,DWORD [36+EDX]
	ADD	EAX,52
	PUSH	EAX
	CALL	_fifo32_put
	POP	EDI
	POP	EAX
L36:
	CMP	DWORD [-1040+EBP],271
	JE	L114
L37:
	MOV	EAX,DWORD [-1040+EBP]
	CMP	EAX,298
	JE	L115
L39:
	CMP	EAX,310
	JE	L116
L40:
	CMP	EAX,426
	JE	L117
L41:
	CMP	EAX,438
	JE	L118
L42:
	CMP	EAX,314
	JE	L119
L43:
	CMP	DWORD [-1040+EBP],325
	JE	L120
L44:
	CMP	DWORD [-1040+EBP],326
	JE	L121
L45:
	CMP	DWORD [-1040+EBP],315
	JE	L122
L46:
	CMP	DWORD [-1040+EBP],316
	JE	L123
L48:
	CMP	DWORD [-1040+EBP],343
	JE	L124
L50:
	MOV	EAX,DWORD [-1040+EBP]
	CMP	EAX,506
	JE	L125
L51:
	CMP	EAX,510
	JNE	L14
	CALL	_wait_KBC_sendready
	PUSH	DWORD [-1092+EBP]
	PUSH	96
	CALL	_io_out8
	POP	ESI
	POP	EDI
	JMP	L14
L125:
	MOV	DWORD [-1092+EBP],-1
	JMP	L51
L124:
	MOV	EDX,DWORD [-1044+EBP]
	MOV	EAX,DWORD [16+EDX]
	DEC	EAX
	PUSH	EAX
	PUSH	DWORD [24+EDX]
	CALL	_sheet_updown
	POP	EAX
	POP	EDX
	JMP	L50
L123:
	CMP	DWORD [-1084+EBP],0
	JE	L48
	CMP	DWORD [-1112+EBP],0
	JE	L49
	PUSH	DWORD [-1112+EBP]
	CALL	_keywin_off
	POP	ECX
L49:
	PUSH	DWORD [-1072+EBP]
	PUSH	DWORD [-1044+EBP]
	CALL	_open_console
	PUSH	4
	PUSH	32
	PUSH	EAX
	MOV	DWORD [-1112+EBP],EAX
	CALL	_sheet_slide
	MOV	EAX,DWORD [-1044+EBP]
	PUSH	DWORD [16+EAX]
	PUSH	DWORD [-1112+EBP]
	CALL	_sheet_updown
	PUSH	DWORD [-1112+EBP]
	CALL	_keywin_on
	ADD	ESP,32
	JMP	L48
L122:
	CMP	DWORD [-1084+EBP],0
	JE	L46
	CMP	DWORD [-1112+EBP],0
	JE	L46
	MOV	EDX,DWORD [-1112+EBP]
	MOV	EBX,DWORD [36+EDX]
	TEST	EBX,EBX
	JE	L46
	CMP	DWORD [92+EBX],0
	JE	L46
	PUSH	LC5
	PUSH	DWORD [204+EBX]
	CALL	_cons_putstr0
	CALL	_io_cli
	LEA	EAX,DWORD [88+EBX]
	MOV	DWORD [124+EBX],EAX
	MOV	DWORD [116+EBX],_asm_end_app
	CALL	_io_sti
	PUSH	0
	PUSH	-1
	PUSH	EBX
	CALL	_task_run
	ADD	ESP,20
	JMP	L46
L121:
	PUSH	237
	LEA	EBX,DWORD [-124+EBP]
	PUSH	EBX
	XOR	DWORD [-1088+EBP],1
	CALL	_fifo32_put
	PUSH	DWORD [-1088+EBP]
	PUSH	EBX
	CALL	_fifo32_put
	ADD	ESP,16
	JMP	L45
L120:
	PUSH	237
	LEA	EBX,DWORD [-124+EBP]
	PUSH	EBX
	XOR	DWORD [-1088+EBP],2
	CALL	_fifo32_put
	PUSH	DWORD [-1088+EBP]
	PUSH	EBX
	CALL	_fifo32_put
	ADD	ESP,16
	JMP	L44
L119:
	PUSH	237
	LEA	EBX,DWORD [-124+EBP]
	PUSH	EBX
	XOR	DWORD [-1088+EBP],4
	CALL	_fifo32_put
	PUSH	DWORD [-1088+EBP]
	PUSH	EBX
	CALL	_fifo32_put
	ADD	ESP,16
	JMP	L43
L118:
	AND	DWORD [-1084+EBP],-3
	JMP	L42
L117:
	AND	DWORD [-1084+EBP],-2
	JMP	L41
L116:
	OR	DWORD [-1084+EBP],2
	JMP	L40
L115:
	OR	DWORD [-1084+EBP],1
	JMP	L39
L114:
	CMP	DWORD [-1112+EBP],0
	JE	L37
	PUSH	DWORD [-1112+EBP]
	CALL	_keywin_off
	MOV	EAX,DWORD [-1112+EBP]
	POP	ESI
	MOV	EBX,DWORD [24+EAX]
	DEC	EBX
	JNE	L38
	MOV	EDX,DWORD [-1044+EBP]
	MOV	EBX,DWORD [16+EDX]
	DEC	EBX
L38:
	MOV	EAX,DWORD [-1044+EBP]
	MOV	EBX,DWORD [20+EAX+EBX*4]
	PUSH	EBX
	MOV	DWORD [-1112+EBP],EBX
	CALL	_keywin_on
	POP	EBX
	JMP	L37
L34:
	LEA	EAX,DWORD [32+EDX]
	MOV	BYTE [-60+EBP],AL
	JMP	L32
L97:
	CMP	DWORD [-1084+EBP],0
	JE	L32
	JMP	L34
L29:
	MOV	AL,BYTE [_keytable1.1-256+EDX]
	JMP	L101
L28:
	MOV	BYTE [-60+EBP],0
	JMP	L31
L105:
	CMP	DWORD [-1056+EBP],0
	JNS	L126
	CMP	DWORD [-1064+EBP],2147483647
	JNE	L127
	PUSH	DWORD [-1080+EBP]
	CALL	_task_sleep
	CALL	_io_sti
	JMP	L99
L127:
	CALL	_io_sti
	PUSH	DWORD [-1068+EBP]
	PUSH	DWORD [-1064+EBP]
	PUSH	DWORD [-1108+EBP]
	CALL	_sheet_slide
	MOV	DWORD [-1064+EBP],2147483647
	JMP	L100
L126:
	CALL	_io_sti
	PUSH	DWORD [-1060+EBP]
	PUSH	DWORD [-1056+EBP]
	PUSH	DWORD [-1076+EBP]
	CALL	_sheet_slide
	MOV	DWORD [-1056+EBP],-1
	JMP	L100
L104:
	PUSH	EBX
	CALL	_fifo32_get
	MOV	DWORD [-1092+EBP],EAX
	CALL	_wait_KBC_sendready
	PUSH	DWORD [-1092+EBP]
	PUSH	96
	CALL	_io_out8
	ADD	ESP,12
	JMP	L17
L2:
	PUSH	145472
	PUSH	3932160
	CALL	_memman_alloc_4k
	XOR	EDX,EDX
	POP	ESI
	MOV	ECX,EAX
	MOV	DWORD [-1040+EBP],0
	POP	EDI
L8:
	MOV	AL,BYTE [_hankaku+EDX]
	MOV	BYTE [EDX+ECX*1],AL
	MOV	EAX,DWORD [-1040+EBP]
	INC	EAX
	MOV	DWORD [-1040+EBP],EAX
	MOV	EDX,EAX
	CMP	EAX,4095
	JLE	L8
	MOV	DWORD [-1040+EBP],4096
	MOV	EAX,4096
L13:
	MOV	BYTE [EAX+ECX*1],-1
	MOV	EAX,DWORD [-1040+EBP]
	INC	EAX
	MOV	DWORD [-1040+EBP],EAX
	CMP	EAX,145471
	JLE	L13
	JMP	L3
	GLOBAL	_keywin_off
_keywin_off:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EBX
	PUSH	0
	MOV	EBX,DWORD [8+EBP]
	PUSH	EBX
	CALL	_change_wtitle8
	TEST	BYTE [28+EBX],32
	POP	ECX
	POP	EAX
	JNE	L130
L128:
	MOV	EBX,DWORD [-4+EBP]
	LEAVE
	RET
L130:
	PUSH	3
	MOV	EAX,DWORD [36+EBX]
	ADD	EAX,52
	PUSH	EAX
	CALL	_fifo32_put
	POP	EAX
	POP	EDX
	JMP	L128
	GLOBAL	_keywin_on
_keywin_on:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EBX
	PUSH	1
	MOV	EBX,DWORD [8+EBP]
	PUSH	EBX
	CALL	_change_wtitle8
	TEST	BYTE [28+EBX],32
	POP	ECX
	POP	EAX
	JNE	L133
L131:
	MOV	EBX,DWORD [-4+EBP]
	LEAVE
	RET
L133:
	PUSH	2
	MOV	EAX,DWORD [36+EBX]
	ADD	EAX,52
	PUSH	EAX
	CALL	_fifo32_put
	POP	EAX
	POP	EDX
	JMP	L131
	GLOBAL	_open_constask
_open_constask:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	ESI
	PUSH	EBX
	CALL	_task_alloc
	PUSH	512
	PUSH	3932160
	MOV	EBX,EAX
	CALL	_memman_alloc_4k
	PUSH	65536
	PUSH	3932160
	MOV	ESI,EAX
	CALL	_memman_alloc_4k
	MOV	EDX,DWORD [8+EBP]
	MOV	DWORD [116+EBX],_console_task
	MOV	DWORD [216+EBX],EAX
	ADD	EAX,65524
	MOV	DWORD [140+EBX],EAX
	MOV	DWORD [156+EBX],8
	MOV	DWORD [160+EBX],16
	MOV	DWORD [164+EBX],8
	MOV	DWORD [168+EBX],8
	MOV	DWORD [172+EBX],8
	MOV	DWORD [176+EBX],8
	MOV	DWORD [4+EAX],EDX
	MOV	EDX,DWORD [140+EBX]
	MOV	EAX,DWORD [12+EBP]
	MOV	DWORD [8+EDX],EAX
	PUSH	2
	PUSH	2
	PUSH	EBX
	CALL	_task_run
	LEA	EAX,DWORD [52+EBX]
	PUSH	EBX
	PUSH	ESI
	PUSH	128
	PUSH	EAX
	CALL	_fifo32_init
	LEA	ESP,DWORD [-8+EBP]
	MOV	EAX,EBX
	POP	EBX
	POP	ESI
	POP	EBP
	RET
	GLOBAL	_open_log_task
_open_log_task:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	ESI
	PUSH	EBX
	CALL	_task_alloc
	PUSH	512
	PUSH	3932160
	MOV	EBX,EAX
	CALL	_memman_alloc_4k
	PUSH	65536
	PUSH	3932160
	MOV	ESI,EAX
	CALL	_memman_alloc_4k
	MOV	EDX,DWORD [8+EBP]
	MOV	DWORD [116+EBX],_log_task
	MOV	DWORD [216+EBX],EAX
	ADD	EAX,65524
	MOV	DWORD [140+EBX],EAX
	MOV	DWORD [156+EBX],8
	MOV	DWORD [160+EBX],16
	MOV	DWORD [164+EBX],8
	MOV	DWORD [168+EBX],8
	MOV	DWORD [172+EBX],8
	MOV	DWORD [176+EBX],8
	MOV	DWORD [4+EAX],EDX
	MOV	EDX,DWORD [140+EBX]
	MOV	EAX,DWORD [12+EBP]
	MOV	DWORD [8+EDX],EAX
	MOV	DWORD [12+EBX],6778732
	PUSH	2
	PUSH	2
	PUSH	EBX
	CALL	_task_run
	LEA	EAX,DWORD [52+EBX]
	PUSH	EBX
	PUSH	ESI
	PUSH	128
	PUSH	EAX
	CALL	_fifo32_init
	LEA	ESP,DWORD [-8+EBP]
	MOV	EAX,EBX
	POP	EBX
	POP	ESI
	POP	EBP
	RET
[SECTION .data]
LC6:
	DB	"console",0x00
[SECTION .text]
	GLOBAL	_open_console
_open_console:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	ESI
	PUSH	EBX
	PUSH	DWORD [8+EBP]
	CALL	_sheet_alloc
	PUSH	343824
	PUSH	3932160
	MOV	EBX,EAX
	CALL	_memman_alloc_4k
	PUSH	-1
	PUSH	741
	MOV	ESI,EAX
	PUSH	464
	PUSH	EAX
	PUSH	EBX
	CALL	_sheet_setbuf
	ADD	ESP,32
	PUSH	0
	PUSH	LC6
	PUSH	741
	PUSH	464
	PUSH	ESI
	CALL	_make_window8
	PUSH	0
	PUSH	704
	PUSH	448
	PUSH	28
	PUSH	8
	PUSH	EBX
	CALL	_make_textbox8
	ADD	ESP,44
	PUSH	DWORD [12+EBP]
	PUSH	EBX
	CALL	_open_constask
	OR	DWORD [28+EBX],32
	MOV	DWORD [36+EBX],EAX
	LEA	ESP,DWORD [-8+EBP]
	MOV	EAX,EBX
	POP	EBX
	POP	ESI
	POP	EBP
	RET
[SECTION .data]
LC7:
	DB	"log",0x00
[SECTION .text]
	GLOBAL	_open_log_console
_open_log_console:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	ESI
	PUSH	EBX
	PUSH	DWORD [8+EBP]
	CALL	_sheet_alloc
	PUSH	343824
	PUSH	3932160
	MOV	EBX,EAX
	CALL	_memman_alloc_4k
	PUSH	-1
	PUSH	741
	MOV	ESI,EAX
	PUSH	464
	PUSH	EAX
	PUSH	EBX
	CALL	_sheet_setbuf
	ADD	ESP,32
	PUSH	0
	PUSH	LC7
	PUSH	741
	PUSH	464
	PUSH	ESI
	CALL	_make_window8
	PUSH	0
	PUSH	704
	PUSH	448
	PUSH	28
	PUSH	8
	PUSH	EBX
	CALL	_make_textbox8
	ADD	ESP,44
	PUSH	DWORD [12+EBP]
	PUSH	EBX
	CALL	_open_log_task
	OR	DWORD [28+EBX],32
	MOV	DWORD [36+EBX],EAX
	LEA	ESP,DWORD [-8+EBP]
	MOV	EAX,EBX
	POP	EBX
	POP	ESI
	POP	EBP
	RET
	GLOBAL	_close_constask
_close_constask:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EBX
	MOV	EBX,DWORD [8+EBP]
	PUSH	EBX
	CALL	_task_sleep
	PUSH	65536
	PUSH	DWORD [216+EBX]
	PUSH	3932160
	CALL	_memman_free_4k
	PUSH	512
	PUSH	DWORD [52+EBX]
	PUSH	3932160
	CALL	_memman_free_4k
	MOV	DWORD [40+EBX],0
	MOV	EBX,DWORD [-4+EBP]
	LEAVE
	RET
	GLOBAL	_close_console
_close_console:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	ESI
	PUSH	EBX
	MOV	EBX,DWORD [8+EBP]
	MOV	ESI,DWORD [36+EBX]
	PUSH	42240
	PUSH	DWORD [EBX]
	PUSH	3932160
	CALL	_memman_free_4k
	PUSH	EBX
	CALL	_sheet_free
	MOV	DWORD [8+EBP],ESI
	ADD	ESP,16
	LEA	ESP,DWORD [-8+EBP]
	POP	EBX
	POP	ESI
	POP	EBP
	JMP	_close_constask
[SECTION .data]
	ALIGNB	4
_table.2:
	DD	3
	DD	1
	DD	0
	DD	2
[SECTION .text]
	GLOBAL	_rgb2pal
_rgb2pal:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EDI
	PUSH	ESI
	MOV	EDI,256
	PUSH	EBX
	MOV	ESI,256
	SUB	ESP,20
	IMUL	ECX,DWORD [8+EBP],21
	MOV	EDX,DWORD [20+EBP]
	MOV	EAX,DWORD [24+EBP]
	AND	EDX,1
	AND	EAX,1
	LEA	EAX,DWORD [EDX+EAX*2]
	MOV	EBX,DWORD [_table.2+EAX*4]
	MOV	EAX,ECX
	CDQ
	IDIV	ESI
	IMUL	ESI,DWORD [12+EBP],21
	MOV	ECX,EAX
	MOV	EAX,ESI
	CDQ
	IDIV	EDI
	IMUL	ESI,DWORD [16+EBP],21
	MOV	DWORD [-32+EBP],EAX
	ADD	ECX,EBX
	MOV	EAX,ESI
	CDQ
	IDIV	EDI
	MOV	ESI,EAX
	MOV	EDI,4
	MOV	EAX,ECX
	CDQ
	IDIV	EDI
	MOV	ECX,EAX
	MOV	EAX,DWORD [-32+EBP]
	ADD	EAX,EBX
	LEA	EBX,DWORD [EBX+ESI*1]
	CDQ
	IDIV	EDI
	MOV	DWORD [-32+EBP],EAX
	MOV	EAX,EBX
	CDQ
	IDIV	EDI
	MOV	EBX,EAX
	IMUL	EAX,DWORD [-32+EBP],6
	IMUL	EDX,EBX,36
	ADD	ESP,20
	LEA	EAX,DWORD [EAX+ECX*1]
	POP	EBX
	POP	ESI
	LEA	EAX,DWORD [16+EDX+EAX*1]
	POP	EDI
	MOVZX	EAX,AL
	POP	EBP
	RET
[SECTION .data]
LC8:
	DB	"night.bmp",0x00
LC9:
	DB	"found %s, clustno = %d, size = %d",0x00
LC10:
	DB	"load file contents, x = %d, y = %d, info[0] = %d",0x00
LC11:
	DB	"parse image, i = %d",0x00
[SECTION .text]
	GLOBAL	_load_background_pic
_load_background_pic:
	PUSH	EBP
	MOV	EAX,425688
	MOV	EBP,ESP
	PUSH	EDI
	PUSH	ESI
	PUSH	EBX
	LEA	EBX,DWORD [-65580+EBP]
	CALL	__alloca
	PUSH	LC8
	PUSH	EBX
	CALL	_sprintf
	POP	EAX
	POP	EDX
	MOV	DWORD [-425688+EBP],EBX
	PUSH	EBX
	LEA	EBX,DWORD [-425676+EBP]
	PUSH	EBX
	CALL	_sprintf
	PUSH	216
	PUSH	600
	PUSH	184
	PUSH	10
	PUSH	15
	MOVSX	EAX,WORD [4084]
	PUSH	EAX
	PUSH	DWORD [4088]
	CALL	_boxfill8
	ADD	ESP,36
	PUSH	EBX
	PUSH	0
	PUSH	184
	PUSH	10
	MOVSX	EAX,WORD [4084]
	PUSH	EAX
	PUSH	DWORD [4088]
	CALL	_putfonts8_asc
	PUSH	224
	PUSH	1058304
	PUSH	DWORD [-425688+EBP]
	CALL	_file_search
	ADD	ESP,36
	MOV	ESI,EAX
	TEST	EAX,EAX
	JE	L141
	PUSH	DWORD [28+EAX]
	MOVZX	EAX,WORD [26+EAX]
	PUSH	EAX
	PUSH	DWORD [-425688+EBP]
	PUSH	LC9
	PUSH	EBX
	CALL	_sprintf
	PUSH	216
	PUSH	600
	PUSH	200
	PUSH	10
	PUSH	15
	MOVSX	EAX,WORD [4084]
	PUSH	EAX
	PUSH	DWORD [4088]
	CALL	_boxfill8
	ADD	ESP,48
	PUSH	EBX
	LEA	EBX,DWORD [-65548+EBP]
	PUSH	0
	PUSH	200
	PUSH	10
	MOVSX	EAX,WORD [4084]
	PUSH	EAX
	PUSH	DWORD [4088]
	CALL	_putfonts8_asc
	MOV	EAX,DWORD [28+ESI]
	MOV	DWORD [-425680+EBP],EAX
	LEA	EAX,DWORD [-425680+EBP]
	PUSH	DWORD [12+EBP]
	PUSH	EAX
	MOVZX	EAX,WORD [26+ESI]
	PUSH	EAX
	LEA	ESI,DWORD [-65612+EBP]
	CALL	_file_loadfile2
	ADD	ESP,36
	PUSH	EAX
	MOV	DWORD [-425684+EBP],EAX
	PUSH	DWORD [-425680+EBP]
	PUSH	ESI
	PUSH	EBX
	CALL	_info_BMP
	ADD	ESP,16
	TEST	EAX,EAX
	JNE	L144
	PUSH	DWORD [-425684+EBP]
	PUSH	DWORD [-425680+EBP]
	PUSH	ESI
	PUSH	EBX
	CALL	_info_JPEG
	ADD	ESP,16
	TEST	EAX,EAX
	JE	L162
L144:
	PUSH	DWORD [-65612+EBP]
	PUSH	DWORD [-65600+EBP]
	PUSH	DWORD [-65604+EBP]
	PUSH	LC10
	LEA	EBX,DWORD [-425676+EBP]
	PUSH	EBX
	CALL	_sprintf
	PUSH	232
	PUSH	600
	PUSH	216
	PUSH	10
	PUSH	15
	MOVSX	EAX,WORD [4084]
	PUSH	EAX
	PUSH	DWORD [4088]
	CALL	_boxfill8
	ADD	ESP,48
	PUSH	EBX
	PUSH	0
	PUSH	216
	PUSH	10
	MOVSX	EAX,WORD [4084]
	PUSH	EAX
	PUSH	DWORD [4088]
	CALL	_putfonts8_asc
	ADD	ESP,24
	CMP	DWORD [-65604+EBP],500
	JG	L162
	CMP	DWORD [-65600+EBP],400
	JG	L162
	CMP	DWORD [-65612+EBP],1
	JE	L166
	PUSH	0
	LEA	EAX,DWORD [-425612+EBP]
	PUSH	EAX
	LEA	EAX,DWORD [-65548+EBP]
	PUSH	4
	PUSH	DWORD [-425684+EBP]
	PUSH	DWORD [-425680+EBP]
	PUSH	EAX
	CALL	_decode0_JPEG
L165:
	ADD	ESP,24
	LEA	EBX,DWORD [-425676+EBP]
	MOV	EDI,EAX
	PUSH	EAX
	PUSH	LC11
	PUSH	EBX
	CALL	_sprintf
	PUSH	248
	PUSH	600
	PUSH	232
	PUSH	10
	PUSH	15
	MOVSX	EAX,WORD [4084]
	PUSH	EAX
	PUSH	DWORD [4088]
	CALL	_boxfill8
	ADD	ESP,40
	PUSH	EBX
	PUSH	0
	PUSH	232
	PUSH	10
	MOVSX	EAX,WORD [4084]
	PUSH	EAX
	PUSH	DWORD [4088]
	CALL	_putfonts8_asc
	ADD	ESP,24
	TEST	EDI,EDI
	JNE	L162
	MOVSX	EAX,WORD [4086]
	MOV	ESI,2
	LEA	EDX,DWORD [-24+EAX]
	MOV	EAX,EDX
	CDQ
	IDIV	ESI
	MOV	EDX,DWORD [-65600+EBP]
	MOV	EBX,EAX
	MOV	EAX,EDX
	MOV	DWORD [-425700+EBP],EDX
	CDQ
	IDIV	ESI
	MOVSX	ECX,WORD [4084]
	SUB	EBX,EAX
	IMUL	ECX,EBX
	ADD	DWORD [8+EBP],ECX
	CMP	EDI,DWORD [-425700+EBP]
	JGE	L162
L160:
	MOV	EAX,DWORD [4084]
	MOV	EDX,2
	MOVSX	ECX,AX
	IMUL	ECX,EDI
	MOV	EBX,EDX
	MOV	ESI,2
	CWD
	IDIV	BX
	ADD	ECX,DWORD [8+EBP]
	XOR	EBX,EBX
	CWDE
	ADD	ECX,EAX
	MOV	DWORD [-425688+EBP],ECX
	MOV	ECX,DWORD [-65604+EBP]
	MOV	EAX,ECX
	CDQ
	IDIV	ESI
	SUB	DWORD [-425688+EBP],EAX
	MOV	EAX,EDI
	IMUL	EAX,ECX
	CMP	EBX,ECX
	LEA	ESI,DWORD [-425612+EBP+EAX*4]
	JL	L159
L164:
	INC	EDI
	CMP	EDI,DWORD [-65600+EBP]
	JL	L160
L162:
	PUSH	DWORD [-425680+EBP]
	PUSH	DWORD [-425684+EBP]
	PUSH	3932160
	CALL	_memman_free_4k
L141:
	LEA	ESP,DWORD [-12+EBP]
	POP	EBX
	POP	ESI
	POP	EDI
	POP	EBP
	RET
L159:
	PUSH	EDI
	PUSH	EBX
	MOVZX	EAX,BYTE [ESI+EBX*4]
	PUSH	EAX
	MOVZX	EAX,BYTE [1+ESI+EBX*4]
	PUSH	EAX
	MOVZX	EAX,BYTE [2+ESI+EBX*4]
	PUSH	EAX
	CALL	_rgb2pal
	ADD	ESP,20
	MOV	EDX,DWORD [-425688+EBP]
	MOV	BYTE [EBX+EDX*1],AL
	INC	EBX
	CMP	EBX,DWORD [-65604+EBP]
	JL	L159
	JMP	L164
L166:
	PUSH	0
	LEA	EAX,DWORD [-425612+EBP]
	PUSH	EAX
	LEA	EAX,DWORD [-65548+EBP]
	PUSH	4
	PUSH	DWORD [-425684+EBP]
	PUSH	DWORD [-425680+EBP]
	PUSH	EAX
	CALL	_decode0_BMP
	JMP	L165
	GLOBAL	_print_on_screen2
_print_on_screen2:
	PUSH	EBP
	OR	ECX,-1
	MOV	EBP,ESP
	MOV	EDX,DWORD [_bootinfo]
	PUSH	EDI
	PUSH	ESI
	MOV	EDI,DWORD [8+EBP]
	PUSH	EBX
	MOV	ESI,DWORD [12+EBP]
	MOV	EBX,DWORD [16+EBP]
	CLD
	LEA	EAX,DWORD [16+EBX]
	PUSH	EAX
	XOR	EAX,EAX
	REPNE
	SCASB
	NOT	ECX
	LEA	ECX,DWORD [-8+ESI+ECX*8]
	PUSH	ECX
	PUSH	EBX
	PUSH	ESI
	PUSH	15
	MOVSX	EAX,WORD [4+EDX]
	PUSH	EAX
	PUSH	DWORD [8+EDX]
	CALL	_boxfill8
	MOV	EDX,DWORD [_bootinfo]
	PUSH	DWORD [8+EBP]
	PUSH	0
	PUSH	EBX
	PUSH	ESI
	MOVSX	EAX,WORD [4+EDX]
	PUSH	EAX
	PUSH	DWORD [8+EDX]
	CALL	_putfonts8_asc
	LEA	ESP,DWORD [-12+EBP]
	POP	EBX
	POP	ESI
	POP	EDI
	POP	EBP
	RET
[SECTION .data]
	ALIGNB	4
_invoke_level.3:
	DD	0
LC12:
	DB	"%s",0x0A,0x00
[SECTION .text]
	GLOBAL	_debug
_debug:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EBX
	SUB	ESP,2064
	LEA	EAX,DWORD [12+EBP]
	LEA	EBX,DWORD [-1028+EBP]
	PUSH	EAX
	INC	DWORD [_invoke_level.3]
	PUSH	DWORD [8+EBP]
	PUSH	EBX
	CALL	_vsprintf
	PUSH	EBX
	PUSH	LC12
	LEA	EBX,DWORD [-2068+EBP]
	PUSH	EBX
	CALL	_sprintf
	MOV	EAX,DWORD [_log_win]
	ADD	ESP,24
	TEST	EAX,EAX
	JNE	L170
L169:
	MOV	EBX,DWORD [-4+EBP]
	DEC	DWORD [_invoke_level.3]
	LEAVE
	RET
L170:
	PUSH	EBX
	MOV	EAX,DWORD [36+EAX]
	PUSH	DWORD [204+EAX]
	CALL	_cons_putstr0
	POP	ECX
	POP	EBX
	JMP	L169
[SECTION .data]
LC13:
	DB	"%c !!panic!! %s",0x00
[SECTION .text]
	GLOBAL	_panic
_panic:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EBX
	SUB	ESP,512
	LEA	EAX,DWORD [12+EBP]
	LEA	EBX,DWORD [-260+EBP]
	PUSH	EAX
	PUSH	DWORD [8+EBP]
	PUSH	EBX
	CALL	_vsprintf
	PUSH	EBX
	PUSH	2
	LEA	EBX,DWORD [-516+EBP]
	PUSH	LC13
	PUSH	EBX
	CALL	_sprintf
	PUSH	EBX
	CALL	_print_on_screen
	ADD	ESP,32
	CALL	_ud2
	MOV	EBX,DWORD [-4+EBP]
	LEAVE
	RET
[SECTION .data]
LC14:
	DB	"----------------TSS begin---------------------------",0x00
LC15:
	DB	"es = %d, cs = %d",0x00
LC16:
	DB	"ss = %d, ds = %d",0x00
LC17:
	DB	"fs = %d, gs = %d",0x00
LC18:
	DB	"backlink = %d",0x00
LC19:
	DB	"esp0 = %d, ss0 = %d",0x00
LC20:
	DB	"esp1 = %d, ss1 = %d",0x00
LC21:
	DB	"esp2 = %d, ss2 = %d",0x00
LC22:
	DB	"eip = %d, esp= %d",0x00
LC23:
	DB	"cr3 = %d",0x00
LC24:
	DB	"----------------TSS end  ---------------------------",0x00
[SECTION .text]
	GLOBAL	_printTSSInfo
_printTSSInfo:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EBX
	MOV	EBX,DWORD [8+EBP]
	PUSH	LC14
	CALL	_debug
	PUSH	DWORD [76+EBX]
	PUSH	DWORD [72+EBX]
	PUSH	LC15
	CALL	_debug
	PUSH	DWORD [84+EBX]
	PUSH	DWORD [80+EBX]
	PUSH	LC16
	CALL	_debug
	PUSH	DWORD [92+EBX]
	PUSH	DWORD [88+EBX]
	PUSH	LC17
	CALL	_debug
	ADD	ESP,40
	PUSH	DWORD [EBX]
	PUSH	LC18
	CALL	_debug
	PUSH	DWORD [8+EBX]
	PUSH	DWORD [4+EBX]
	PUSH	LC19
	CALL	_debug
	PUSH	DWORD [16+EBX]
	PUSH	DWORD [12+EBX]
	PUSH	LC20
	CALL	_debug
	ADD	ESP,32
	PUSH	DWORD [24+EBX]
	PUSH	DWORD [20+EBX]
	PUSH	LC21
	CALL	_debug
	PUSH	DWORD [56+EBX]
	PUSH	DWORD [32+EBX]
	PUSH	LC22
	CALL	_debug
	PUSH	DWORD [28+EBX]
	PUSH	LC23
	CALL	_debug
	MOV	EBX,DWORD [-4+EBP]
	ADD	ESP,32
	MOV	DWORD [8+EBP],LC24
	LEAVE
	JMP	_debug
[SECTION .data]
	ALIGNB	4
_invoke_count.4:
	DD	0
[SECTION .text]
	GLOBAL	_print_on_screen
_print_on_screen:
	PUSH	EBP
	MOV	EAX,DWORD [_invoke_count.4]
	SAL	EAX,4
	MOV	EBP,ESP
	PUSH	EDI
	OR	ECX,-1
	PUSH	EBX
	LEA	EDX,DWORD [200+EAX]
	MOV	EBX,DWORD [8+EBP]
	ADD	EAX,216
	CLD
	PUSH	EAX
	MOV	EDI,EBX
	XOR	EAX,EAX
	REPNE
	SCASB
	NOT	ECX
	LEA	ECX,DWORD [2+ECX*8]
	PUSH	ECX
	PUSH	EDX
	MOV	EDX,DWORD [_bootinfo]
	PUSH	10
	PUSH	15
	MOVSX	EAX,WORD [4+EDX]
	PUSH	EAX
	PUSH	DWORD [8+EDX]
	CALL	_boxfill8
	MOV	EDX,DWORD [_bootinfo]
	MOV	EAX,DWORD [_invoke_count.4]
	PUSH	EBX
	SAL	EAX,4
	PUSH	0
	ADD	EAX,200
	PUSH	EAX
	PUSH	10
	MOVSX	EAX,WORD [4+EDX]
	PUSH	EAX
	PUSH	DWORD [8+EDX]
	CALL	_putfonts8_asc
	ADD	ESP,52
	MOV	EAX,DWORD [_invoke_count.4]
	INC	EAX
	MOV	DWORD [_invoke_count.4],EAX
	CMP	EAX,35
	JE	L175
L173:
	LEA	ESP,DWORD [-8+EBP]
	POP	EBX
	POP	EDI
	POP	EBP
	RET
L175:
	PUSH	968
	MOV	EDX,DWORD [_bootinfo]
	PUSH	1034
	PUSH	200
	PUSH	10
	PUSH	14
	MOVSX	EAX,WORD [4+EDX]
	PUSH	EAX
	PUSH	DWORD [8+EDX]
	MOV	DWORD [_invoke_count.4],0
	CALL	_boxfill8
	MOV	EDX,DWORD [_bootinfo]
	MOV	EAX,DWORD [_invoke_count.4]
	PUSH	EBX
	SAL	EAX,4
	PUSH	0
	ADD	EAX,200
	PUSH	EAX
	PUSH	10
	MOVSX	EAX,WORD [4+EDX]
	PUSH	EAX
	PUSH	DWORD [8+EDX]
	CALL	_putfonts8_asc
	ADD	ESP,52
	JMP	L173
[SECTION .data]
LC25:
	DB	0x0A,"spinning in %s ...",0x0A,0x00
[SECTION .text]
	GLOBAL	_spin
_spin:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	DWORD [8+EBP]
	PUSH	LC25
	CALL	_debug
	POP	EAX
	POP	EDX
L177:
	JMP	L177
[SECTION .data]
LC26:
	DB	"%x ",0x00
[SECTION .text]
	GLOBAL	_string_memory
_string_memory:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	EDI
	PUSH	ESI
	PUSH	EBX
	MOV	ESI,DWORD [16+EBP]
	XOR	EBX,EBX
	CMP	EBX,DWORD [12+EBP]
	JL	L185
L187:
	LEA	ESP,DWORD [-12+EBP]
	POP	EBX
	POP	ESI
	POP	EDI
	POP	EBP
	RET
L185:
	MOV	EDX,DWORD [8+EBP]
	OR	ECX,-1
	CLD
	MOVZX	EAX,BYTE [EBX+EDX*1]
	PUSH	EAX
	MOV	EDI,ESI
	PUSH	LC26
	XOR	EAX,EAX
	REPNE
	SCASB
	NOT	ECX
	INC	EBX
	LEA	ECX,DWORD [-1+ESI+ECX*1]
	PUSH	ECX
	CALL	_sprintf
	ADD	ESP,12
	CMP	EBX,DWORD [12+EBP]
	JL	L185
	JMP	L187
[SECTION .data]
LC27:
	DB	"%c  assert(%s) failed: file: %s, base_file: %s, ln%d",0x00
LC28:
	DB	"assertion_failure()",0x00
[SECTION .text]
	GLOBAL	_assertion_failure
_assertion_failure:
	PUSH	EBP
	MOV	EBP,ESP
	PUSH	DWORD [20+EBP]
	PUSH	DWORD [16+EBP]
	PUSH	DWORD [12+EBP]
	PUSH	DWORD [8+EBP]
	PUSH	3
	PUSH	LC27
	CALL	_debug
	ADD	ESP,24
	MOV	DWORD [8+EBP],LC28
	LEAVE
	JMP	_spin