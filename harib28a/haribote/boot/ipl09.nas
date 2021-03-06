; haribote-ipl
; TAB=4

CYLS	EQU		20				; ﾒｪｶﾁﾈ｡ｶ猖ﾙﾄﾚﾈﾝ

		ORG		0x7c00			; このプログラムがどこに読み込まれるのか

; 以下は標準的なFAT12フォーマットフロッピーディスクのための記述

		JMP		entry
		DB		0x90
		DB		"HARIBOTE"		; ブートセクタの名前を自由に書いてよい（8バイト）
		DW		512				; 1セクタの大きさ（512にしなければいけない）
		DB		1				; クラスタの大きさ（1セクタにしなければいけない）
		DW		1				; FATがどこから始まるか（普通は1セクタ目からにする）
		DB		2				; FATの個数（2にしなければいけない）
		DW		224				; ルートディレクトリ領域の大きさ（普通は224エントリにする）
		DW		2880			; このドライブの大きさ（2880セクタにしなければいけない）
		DB		0xf0			; メディアのタイプ（0xf0にしなければいけない）
		DW		9				; FAT領域の長さ（9セクタにしなければいけない）
		DW		18				; 1トラックにいくつのセクタがあるか（18にしなければいけない）
		DW		2				; ヘッドの数（2にしなければいけない）
		DD		0				; パーティションを使ってないのでここは必ず0
		DD		2880			; このドライブ大きさをもう一度書く
		DB		0,0,0x29		; よくわからないけどこの値にしておくといいらしい
		DD		0xffffffff		; たぶんボリュームシリアル番号
		DB		"HARIBOTEOS "	; ディスクの名前（11バイト）
		DB		"FAT12   "		; フォーマットの名前（8バイト）
		RESB	18				; とりあえず18バイトあけておく

; プログラム本体

entry:
		MOV		AX,0			; レジスタ初期化
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX

;   ｶﾁｴﾅﾅﾌ

		MOV		AX,0x0820
		MOV		ES,AX
		MOV		CH,0			; ﾖ�ﾃ�0
		MOV		DH,0			; ｴﾅﾍｷ0
		MOV		CL,2			; ﾉﾈﾇ�2
		MOV		BX,18*2*CYLS-1	; ﾒｪｶﾁﾈ｡ｵﾄｺﾏｼﾆﾉﾈﾇ�ﾊ�｣ｬｴﾓｴﾋｿｪﾊｼ
		CALL	readfast		; ｸ賤ﾟｶﾁﾈ｡

; ｶﾁﾈ｡ｽ睫�｣ｬﾔﾋﾐﾐharibote.sys

		MOV		BYTE [0x0ff0],CYLS	; ｼﾇﾂｼIPLﾊｵｼﾊｶﾁﾈ｡ﾁﾋｶ猖ﾙﾄﾚﾈﾝ｣ｬｵｽｴﾋｽ睫�
		JMP		0xc200

error:
		MOV		AX,0
		MOV		ES,AX
		MOV		SI,msg
putloop:
		MOV		AL,[SI]
		ADD		SI,1			; SIｼﾓ1
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; ﾏﾔﾊｾﾒｻｸ�ﾗﾖｷ�
		MOV		BX,15			; ﾑﾕﾉｫｴ�ﾂ�
		INT		0x10			; ｵ�ﾓﾃﾏﾔﾊｾBIOS
		JMP		putloop
fin:
		HLT						; ﾔﾝﾊｱﾈﾃCPUﾍ｣ﾖｹ
		JMP		fin				; ﾎﾞﾏﾞﾑｭｻｷ
msg:
		DB		0x0a, 0x0a		; ﾁｽｸ�ｻｻﾐﾐ
		DB		"load error"
		DB		0x0a			; ｻｻﾐﾐ
		DB		0

readfast:	; ﾊｹﾓﾃALｾ｡ﾁｿﾒｻｴﾎﾐﾔｶﾁﾈ｡ﾊ�ｾﾝ｣ｬｴﾓｴﾋｿｪﾊｼ
;	ES:読み込み番地, CH:ﾖ�ﾃ�, DH:ｴﾅﾍｷ, CL:ﾉﾈﾇ�, BX:ｶﾁﾈ｡ﾉﾈﾇ�ﾊ�

		MOV		AX,ES			; < ﾍｨｹ�ESｼﾆﾋ紜Lｵﾄﾗ�ｴ�ﾖｵ >
		SHL		AX,3			; ｽｫAXｳ�ﾒﾔ32,ｽｫｽ盪�ｴ貶�AH
		AND		AH,0x7f			; AHﾊﾇAHｳ�ﾒﾔ128ﾋ�ｵﾃｵﾄﾓ猝�｣ｨ512*128=64K)
		MOV		AL,128			; AL = 128 - AH; AHﾊﾇAHｳ�ﾒﾔ128ﾋ�ｵﾃｵﾄﾓ猝�
		SUB		AL,AH

		MOV		AH,BL			; < ﾍｨｹ�BXｼﾆﾋ紜Lｵﾄﾗ�ｴ�ﾖｵｲ｢ｴ貶�AH >
		CMP		BH,0			; if (BH != 0) { AH = 18; }
		JE		.skip1
		MOV		AH,18
.skip1:
		CMP		AL,AH			; if (AL > AH) { AL = AH; }
		JBE		.skip2
		MOV		AL,AH
.skip2:

		MOV		AH,19			; < ﾍｨｹ�CLｼﾆﾋ紜Lｵﾄﾗ�ｴ�ﾖｵｲ｢ｴ貶�AH >
		SUB		AH,CL			; AH = 19 - CL;
		CMP		AL,AH			; if (AL > AH) { AL = AH; }
		JBE		.skip3
		MOV		AL,AH
.skip3:

		PUSH	BX
		MOV		SI,0			; ｼﾆﾋ飜ｧｰﾜｴﾎﾊ�ｵﾄｼﾄｴ貳�
retry:
		MOV		AH,0x02			; AH=0x02 : ｶﾁﾈ｡ｴﾅﾅﾌ
		MOV		BX,0
		MOV		DL,0x00			; Aﾅﾌ
		PUSH	ES
		PUSH	DX
		PUSH	CX
		PUSH	AX
		INT		0x13			; ｵ�ﾓﾃｴﾅﾅﾌBIOS
		JNC		next			; ﾃｻﾓﾐｳ�ｴ�ｵﾄｻｰﾔ�ﾌ�ﾗｪﾖﾁnext
		ADD		SI,1			; ｽｫSIｼﾓ1
		CMP		SI,5			; ｽｫSIﾓ�6ｱﾈｽﾏ
		JAE		error			; SI >= 5 ﾔ�ﾌ�ﾗｪﾖﾁerror
		MOV		AH,0x00
		MOV		DL,0x00			; Aﾅﾌ
		INT		0x13			; ﾇ�ｶｯﾆ�ﾖﾘﾖﾃ
		POP		AX
		POP		CX
		POP		DX
		POP		ES
		JMP		retry
next:
		POP		AX
		POP		CX
		POP		DX
		POP		BX				; ｽｫESｵﾄﾄﾚﾈﾝｴ貶�BX
		SHR		BX,5			; ｽｫBXﾓﾉ16ﾗﾖｽﾚﾎｪｵ･ﾎｻﾗｪｻｻﾎｪ512ﾗﾖｽﾚﾎｪｵ･ﾎｻ
		MOV		AH,0
		ADD		BX,AX			; BX += AL;
		SHL		BX,5			; ｽｫBXﾓﾉ512ﾗﾖｽﾚﾎｪｵ･ﾎｻﾗｪｻｻﾎｪ16ﾗﾖｽﾚﾎｪｵ･ﾎｻ
		MOV		ES,BX			; ﾏ犒ｱﾓﾚ ES += AL * 0x20; 
		POP		BX
		SUB		BX,AX
		JZ		.ret
		ADD		CL,AL			; ｽｫCLｼﾓﾉﾏAL
		CMP		CL,18			; ｽｫCLﾓ�18ｱﾈｽﾏ
		JBE		readfast		; CL <= 18 ﾔ�ﾌ�ﾗｪﾖﾁreadfast
		MOV		CL,1
		ADD		DH,1
		CMP		DH,2
		JB		readfast		; DH < 2 ﾔ�ﾌ�ﾗｪﾖﾁreadfast
		MOV		DH,0
		ADD		CH,1
		JMP		readfast
.ret:
		RET						 ;ｵｽｴﾋｽ睫�

		RESB	0x7dfe-$		; ｵｽ0x7dfeﾎｪﾖｹﾓﾃ0x00ﾌ�ｳ莊ﾄﾖｸﾁ�

		DB		0x55, 0xaa
