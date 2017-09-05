#ifndef __LIST_H__
#define __LIST_H__

#include "Types.h"

/***** 구조체 정의 *****/
#pragma pack(push, 1)

typedef struct kListLinkStruct{
	void* pvNext;
	QWORD qwID;
} LISTLINK;

/*
// 리스트 아이템 정의 예시
typedef struct kListItemExampleStruct{
	LISTLINK stLink; // LISTLINK가 반드시 구조체의 맨 앞에 위치해야 함
	int iData1;
	char cData2;
} LISTITEM; // LISTNODE에 해당
*/

typedef struct kListManagerStruct{
	int iItemCount;
	void* pvHead;
	void* pvTail;
} LIST;

#pragma pack(pop)

/***** 함수 정의 *****/
void kInitializeList(LIST* pstList);
int kGetListCount(const LIST* pstList);
void kAddListToTail(LIST* pstList, void* pvItem);
void kAddListToHead(LIST* pstList, void* pvItem);
void* kRemoveList(LIST* pstList, QWORD qwID);
void* kRemoveListFromHead(LIST* pstList);
void* kRemoveListFromTail(LIST* pstList);
void* kFindList(const LIST* pstList, QWORD qwID);
void* kGetHeadFromList(const LIST* pstList);
void* kGetTailFromList(const LIST* pstList);
void* kGetNextFromList(const LIST* pstList, void* pvCurrent);

#endif // __LIST_H__
