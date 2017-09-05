#include "List.h"

void kInitializeList(LIST* pstList){
	pstList->iItemCount = 0;
	pstList->pvHead = NULL;
	pstList->pvTail = NULL;
}

int kGetListCount(const LIST* pstList){
	return pstList->iItemCount;
}

void kAddListToTail(LIST* pstList, void* pvItem){
	LISTLINK* pstLink;

	// ������ζ��, pvItem->stLink->pvNext = NULL;
	// ���� ���� ����������, LISTITEM�� �����̱� ������ �Ʒ��� ���� ������ �Ǿ���
	pstLink = (LISTLINK*)pvItem; // pvItem�� ����Ű�� LISTITEM���� LISTLINK ������ ���� (LISTLINK�� �� �տ� ��ġ�Ѵٴ� ��Ӷ����� ����)
	pstLink->pvNext = NULL;

	// ����Ʈ �������� 0�� ���� ��� (����Ʈ�� ��� ���� ���)
	if(pstList->pvHead == NULL){
		pstList->pvHead = pvItem;
		pstList->pvTail = pvItem;

	// ����Ʈ �������� 1�� �̻� ���� ���
	}else{
		pstLink = (LISTLINK*)pstList->pvTail;
		pstLink->pvNext = pvItem;
		pstList->pvTail = pvItem;
	}

	pstList->iItemCount++;
}

void kAddListToHead(LIST* pstList, void* pvItem){
	LISTLINK* pstLink;

	pstLink = (LISTLINK*)pvItem;
	pstLink->pvNext = pstList->pvHead;

	// ����Ʈ �������� 0�� ���� ��� (����Ʈ�� ��� ���� ���)
	if(pstList->pvHead == NULL){
		pstList->pvHead = pvItem;
		pstList->pvTail = pvItem;

	// ����Ʈ �������� 1�� �̻� ���� ���
	}else{
		pstList->pvHead = pvItem;
	}

	pstList->iItemCount++;
}

void* kRemoveList(LIST* pstList, QWORD qwID){
	LISTLINK* pstLink;
	LISTLINK* pstPrevLink;

	pstPrevLink = (LISTLINK*)pstList->pvHead;
	for(pstLink = pstPrevLink; pstLink != NULL; pstLink = pstLink->pvNext){
		if(pstLink->qwID == qwID){

			// ����Ʈ �������� 1�� ���� ���
			if((pstLink == pstList->pvHead) && (pstLink == pstList->pvTail)){
				pstList->pvHead = NULL;
				pstList->pvTail = NULL;

			// ����Ʈ �������� 2�� �̻� �ְ�, ù��° �������� ���
			}else if(pstLink == pstList->pvHead){
				pstList->pvHead = pstLink->pvNext;

			// ����Ʈ �������� 2�� �̻� �ְ�, ������ �������� ���
			}else if(pstLink == pstList->pvTail){
				pstList->pvTail = pstPrevLink;

			// ����Ʈ �������� 3�� �̻� �ְ�, ��� �������� ���
			}else{
				pstPrevLink->pvNext = pstLink->pvNext;
			}

			pstList->iItemCount--;
			return pstLink;
		}

		pstPrevLink = pstLink;
	}
	return NULL;
}

void* kRemoveListFromHead(LIST* pstList){
	LISTLINK* pstLink;

	// ����Ʈ �������� 0�� ���� ��� (����Ʈ�� ��� ���� ���)
	if(pstList->iItemCount == 0){
		return NULL;
	}

	pstLink = (LISTLINK*)pstList->pvHead;
	return kRemoveList(pstList, pstLink->qwID);
}

void* kRemoveListFromTail(LIST* pstList){
	LISTLINK* pstLink;

	// ����Ʈ �������� 0�� ���� ��� (����Ʈ�� ��� ���� ���)
	if(pstList->iItemCount == 0){
		return NULL;
	}

	pstLink = (LISTLINK*)pstList->pvTail;
	return kRemoveList(pstList, pstLink->qwID);
}

void* kFindList(const LIST* pstList, QWORD qwID){
	LISTLINK* pstLink;

	for(pstLink = (LISTLINK*)pstList->pvHead; pstLink != NULL; pstLink = pstLink->pvNext){
		if(pstLink->qwID == qwID){
			return pstLink;
		}
	}

	return NULL;
}

void* kGetHeadFromList(const LIST* pstList){
	return pstList->pvHead;
}

void* kGetTailFromList(const LIST* pstList){
	return pstList->pvTail;
}

void* kGetNextFromList(const LIST* pstList, void* pvCurrent){
	LISTLINK* pstLink;

	pstLink = (LISTLINK*)pvCurrent;

	return pstLink->pvNext;
}
