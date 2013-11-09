; haribote-ipl
; TAB=4

CYLS	EQU		20				; 要读取多少内容

		ORG		0x7c00			; 偙偺僾儘僌儔儉偑偳偙偵撉傒崬傑傟傞偺偐

; 埲壓偼昗弨揑側FAT12僼僅乕儅僢僩僼儘僢僺乕僨傿僗僋偺偨傔偺婰弎

		JMP		entry
		DB		0x90
		DB		"HARIBOTE"		; 僽乕僩僙僋僞偺柤慜傪帺桼偵彂偄偰傛偄乮8僶僀僩乯
		DW		512				; 1僙僋僞偺戝偒偝乮512偵偟側偗傟偽偄偗側偄乯
		DB		1				; 僋儔僗僞偺戝偒偝乮1僙僋僞偵偟側偗傟偽偄偗側偄乯
		DW		1				; FAT偑偳偙偐傜巒傑傞偐乮晛捠偼1僙僋僞栚偐傜偵偡傞乯
		DB		2				; FAT偺屄悢乮2偵偟側偗傟偽偄偗側偄乯
		DW		224				; 儖乕僩僨傿儗僋僩儕椞堟偺戝偒偝乮晛捠偼224僄儞僩儕偵偡傞乯
		DW		2880			; 偙偺僪儔僀僽偺戝偒偝乮2880僙僋僞偵偟側偗傟偽偄偗側偄乯
		DB		0xf0			; 儊僨傿傾偺僞僀僾乮0xf0偵偟側偗傟偽偄偗側偄乯
		DW		9				; FAT椞堟偺挿偝乮9僙僋僞偵偟側偗傟偽偄偗側偄乯
		DW		18				; 1僩儔僢僋偵偄偔偮偺僙僋僞偑偁傞偐乮18偵偟側偗傟偽偄偗側偄乯
		DW		2				; 僿僢僪偺悢乮2偵偟側偗傟偽偄偗側偄乯
		DD		0				; 僷乕僥傿僔儑儞傪巊偭偰側偄偺偱偙偙偼昁偢0
		DD		2880			; 偙偺僪儔僀僽戝偒偝傪傕偆堦搙彂偔
		DB		0,0,0x29		; 傛偔傢偐傜側偄偗偳偙偺抣偵偟偰偍偔偲偄偄傜偟偄
		DD		0xffffffff		; 偨傇傫儃儕儏乕儉僔儕傾儖斣崋
		DB		"HARIBOTEOS "	; 僨傿僗僋偺柤慜乮11僶僀僩乯
		DB		"FAT12   "		; 僼僅乕儅僢僩偺柤慜乮8僶僀僩乯
		RESB	18				; 偲傝偁偊偢18僶僀僩偁偗偰偍偔

; 僾儘僌儔儉杮懱

entry:
		MOV		AX,0			; 儗僕僗僞弶婜壔
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX

;   读磁盘

		MOV		AX,0x0820
		MOV		ES,AX
		MOV		CH,0			; 柱面0
		MOV		DH,0			; 磁头0
		MOV		CL,2			; 扇区2
		MOV		BX,18*2*CYLS-1	; 撉傒崬傒偨偄崌寁僙僋僞悢
		CALL	readfast		; 崅懍撉傒崬傒

; 撉傒廔傢偭偨偺偱haribote.sys傪幚峴偩両

		MOV		BYTE [0x0ff0],CYLS	; IPL偑偳偙傑偱撉傫偩偺偐傪儊儌
		JMP		0xc200

error:
		MOV		AX,0
		MOV		ES,AX
		MOV		SI,msg
putloop:
		MOV		AL,[SI]
		ADD		SI,1			; SI偵1傪懌偡
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; 堦暥帤昞帵僼傽儞僋僔儑儞
		MOV		BX,15			; 僇儔乕僐乕僪
		INT		0x10			; 價僨僆BIOS屇傃弌偟
		JMP		putloop
fin:
		HLT						; 壗偐偁傞傑偱CPU傪掆巭偝偣傞
		JMP		fin				; 柍尷儖乕僾
msg:
		DB		0x0a, 0x0a		; 夵峴傪2偮
		DB		"load error"
		DB		0x0a			; 夵峴
		DB		0

readfast:	; AL傪巊偭偰偱偒傞偩偗傑偲傔偰撉傒弌偡
;	ES:撉傒崬傒斣抧, CH:僔儕儞僟, DH:僿僢僪, CL:僙僋僞, BX:撉傒崬傒僙僋僞悢

		MOV		AX,ES			; < ES偐傜AL偺嵟戝抣傪寁嶼 >
		SHL		AX,3			; AX傪32偱妱偭偰丄偦偺寢壥傪AH偵擖傟偨偙偲偵側傞 乮SHL偼嵍僔僼僩柦椷乯
		AND		AH,0x7f			; AH偼AH傪128偱妱偭偨梋傝乮512*128=64K乯
		MOV		AL,128			; AL = 128 - AH; 堦斣嬤偄64KB嫬奅傑偱嵟戝壗僙僋僞擖傞偐
		SUB		AL,AH

		MOV		AH,BL			; < BX偐傜AL偺嵟戝抣傪AH偵寁嶼 >
		CMP		BH,0			; if (BH != 0) { AH = 18; }
		JE		.skip1
		MOV		AH,18
.skip1:
		CMP		AL,AH			; if (AL > AH) { AL = AH; }
		JBE		.skip2
		MOV		AL,AH
.skip2:

		MOV		AH,19			; < CL偐傜AL偺嵟戝抣傪AH偵寁嶼 >
		SUB		AH,CL			; AH = 19 - CL;
		CMP		AL,AH			; if (AL > AH) { AL = AH; }
		JBE		.skip3
		MOV		AL,AH
.skip3:

		PUSH	BX
		MOV		SI,0			; 幐攕夞悢傪悢偊傞儗僕僗僞
retry:
		MOV		AH,0x02			; AH=0x02 : 僨傿僗僋撉傒崬傒
		MOV		BX,0
		MOV		DL,0x00			; A僪儔僀僽
		PUSH	ES
		PUSH	DX
		PUSH	CX
		PUSH	AX
		INT		0x13			; 僨傿僗僋BIOS屇傃弌偟
		JNC		next			; 僄儔乕偑偍偒側偗傟偽next傊
		ADD		SI,1			; SI偵1傪懌偡
		CMP		SI,5			; SI偲5傪斾妑
		JAE		error			; SI >= 5 偩偭偨傜error傊
		MOV		AH,0x00
		MOV		DL,0x00			; A僪儔僀僽
		INT		0x13			; 僪儔僀僽偺儕僙僢僩
		POP		AX
		POP		CX
		POP		DX
		POP		ES
		JMP		retry
next:
		POP		AX
		POP		CX
		POP		DX
		POP		BX				; ES偺撪梕傪BX偱庴偗庢傞
		SHR		BX,5			; BX傪16僶僀僩扨埵偐傜512僶僀僩扨埵傊
		MOV		AH,0
		ADD		BX,AX			; BX += AL;
		SHL		BX,5			; BX傪512僶僀僩扨埵偐傜16僶僀僩扨埵傊
		MOV		ES,BX			; 偙傟偱 ES += AL * 0x20; 偵側傞
		POP		BX
		SUB		BX,AX
		JZ		.ret
		ADD		CL,AL			; CL偵AL傪懌偡
		CMP		CL,18			; CL偲18傪斾妑
		JBE		readfast		; CL <= 18 偩偭偨傜readfast傊
		MOV		CL,1
		ADD		DH,1
		CMP		DH,2
		JB		readfast		; DH < 2 偩偭偨傜readfast傊
		MOV		DH,0
		ADD		CH,1
		JMP		readfast
.ret:
		RET

		RESB	0x7dfe-$		; 0x7dfe傑偱傪0x00偱杽傔傞柦椷

		DB		0x55, 0xaa
