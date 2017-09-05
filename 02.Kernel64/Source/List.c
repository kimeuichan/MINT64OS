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

	// 원래대로라면, pvItem->stLink->pvNext = NULL;
	// 위와 같은 로직이지만, LISTITEM이 범용이기 때문에 아래와 같은 로직이 되었음
	pstLink = (LISTLINK*)pvItem; // pvItem이 가리키는 LISTITEM에서 LISTLINK 정보를 추출 (LISTLINK가 맨 앞에 위치한다는 약속때문에 가능)
	pstLink->pvNext = NULL;

	// 리스트 아이템이 0개 있을 경우 (리스트가 비어 있을 경우)
	if(pstList->pvHead == NULL){
		pstList->pvHead = pvItem;
		pstList->pvTail = pvItem;

	// 리스트 아이템이 1개 이상 있을 경우
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

	// 리스트 아이템이 0개 있을 경우 (리스트가 비어 있을 경우)
	if(pstList->pvHead == NULL){
		pstList->pvHead = pvItem;
		pstList->pvTail = pvItem;

	// 리스트 아이템이 1개 이상 있을 경우
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

			// 리스트 아이템이 1개 있을 경우
			if((pstLink == pstList->pvHead) && (pstLink == pstList->pvTail)){
				pstList->pvHead = NULL;
				pstList->pvTail = NULL;

			// 리스트 아이템이 2개 이상 있고, 첫번째 아이템일 경우
			}else if(pstLink == pstList->pvHead){
				pstList->pvHead = pstLink->pvNext;

			// 리스트 아이템이 2개 이상 있고, 마지막 아이템일 경우
			}else if(pstLink == pstList->pvTail){
				pstList->pvTail = pstPrevLink;

			// 리스트 아이템이 3개 이상 있고, 가운데 아이템일 경우
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

	// 리스트 아이템이 0개 있을 경우 (리스트가 비어 있을 경우)
	if(pstList->iItemCount == 0){
		return NULL;
	}

	pstLink = (LISTLINK*)pstList->pvHead;
	return kRemoveList(pstList, pstLink->qwID);
}

void* kRemoveListFromTail(LIST* pstList){
	LISTLINK* pstLink;

	// 리스트 아이템이 0개 있을 경우 (리스트가 비어 있을 경우)
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
