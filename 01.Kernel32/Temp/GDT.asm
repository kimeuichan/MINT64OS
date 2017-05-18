; GDTR 자료 구조 정의
GDTR:
	dw GDTEND - GDT - 1 		; 아래에 위치하는 GDT 테이블의 전체 크기
	dd ( GDT - $$ + 0x1000) 	; 아래에 위취하는 GDT 테이블 시작 어드레스

GDT:
	dw 0x0000
	dw 0x0000
	db 0x00
	db 0x00