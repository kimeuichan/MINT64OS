#ifndef __TYPES_H__
#define __TYPES_H__

/***** ¸ÅÅ©·Î Á¤ÀÇ *****/
#define BYTE  unsigned char
#define WORD  unsigned short
#define DWORD unsigned int
#define QWORD unsigned long
#define BOOL  unsigned char

#define TRUE  1
#define FALSE 0
#define NULL  0

// stddef.h 헤더에 포함된 offsetof() 매크로 사용
#define offsetof(TYPE, MEMBER) __builtin_offsetof(TYPE, MEMBER)

/***** ±¸Á¶Ã¼ Á¤ÀÇ *****/
#pragma pack(push, 1)

typedef struct kCharacterStruct{
	BYTE bCharacter;
	BYTE bAttribute;
} CHARACTER;

#pragma pack(pop)

#endif // __TYPES_H__
