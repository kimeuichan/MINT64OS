#ifndef __TYPES_H__
#define __TYPES_H__

/***** 매크로 정의 *****/
#define BYTE  unsigned char
#define WORD  unsigned short
#define DWORD unsigned int
#define QWORD unsigned long
#define BOOL  unsigned char

#define TRUE  1
#define FALSE 0
#define NULL  0

// 원래 stddef.h 에 포함된 offsetof() 매크로 함수를 타입 중복을 피하기 위해 직접 작성
#define offsetof(TYPE, MEMBER) __builtin_offsetof(TYPE, MEMBER)

/***** 구조체 정의 *****/
#pragma pack(push, 1)

typedef struct kCharacterStruct{
	BYTE bCharacter;
	BYTE bAttribute;
} CHARACTER;

#pragma pack(pop)

#endif // __TYPES_H__
