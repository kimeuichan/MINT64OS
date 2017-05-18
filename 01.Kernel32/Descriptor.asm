CODEDESCRIPTOR:
	dw 0xffff 			; Limit [15:00]
	dw 0x0000			; Base [15:0]
	db 0x00 			; Base [23:16]
	db 0x9a 			; P=1, DPL = 0, Code Segment, Excute/Read
	db 0xcf 			; G=1, D=1, L=0, Limit[19:16]
	db 0x00 			; Base [31:24]