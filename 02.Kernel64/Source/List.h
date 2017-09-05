#ifndef __LIST_H__
#define __LIST_H__

#include "Types.h"

/***** ����ü ���� *****/
#pragma pack(push, 1)

typedef struct kListLinkStruct{
	void* pvNext;
	QWORD qwID;
} LISTLINK;

/*
// ����Ʈ ������ ���� ����
typedef struct kListItemExampleStruct{
	LISTLINK stLink; // LISTLINK�� �ݵ�� ����ü�� �� �տ� ��ġ�ؾ� ��
	int iData1;
	char cData2;
} LISTITEM; // LISTNODE�� �ش�
*/

typedef struct kListManagerStruct{
	int iItemCount;
	void* pvHead;
	void* pvTail;
} LIST;

#pragma pack(pop)

/***** �Լ� ���� *****/
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
