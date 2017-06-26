#include "Queue.h"

void kinitializeQueue(QUEUE* pstQueue, void* pvQueueBuffer, int iMaxDataCount, int iDataSize){
	// 큐의 최대 개수와 크기와 버퍼 어드레스를 저장
	pstQueue->iMaxDataCount = iMaxDataCount;
	pstQueue->iDataSize = iDataSize;
	pstQueue->pvQueueArray = pvQueueBuffer;

	// 큐의 삽입 위치와 제거위치를 초기화하고 마지막으로 수행된 명령을 제거로 설정하여 큐를 빈 상태로 만듬
	pstQueue->iPutIndex = 0;
	pstQueue->iGetIndex = 0;
	pstQueue->bLastOperationPut = FALSE;
}

// 큐가 가득 찼는지 여부를 반환
BOOL kIsQueueFull(const QUEUE* pstQueue){
	// 큐의 삽입 인덱스와 제거 인덱스가 같고 마지막으로 수행된 명령이 삽입이면
	// 큐가 가득 찼으므로 삽입할 수 없음
	if( (pstQueue->iGetIndex == pstQueue->iPutIndex) && (pstQueue->bLastOperationPut == TRUE))
		return TRUE;

	return FALSE;
}

// 큐가 비었는지 여부를 반환
BOOL kIsQueueEmpty(const QUEUE* pstQueue){
	if((pstQueue->iGetIndex == pstQueue->iPutIndex) && (pstQueue->bLastOperationPut == FALSE))
		return TRUE;

	return FALSE;
}

BOOL kPutQueue(QUEUE* pstQueue, const void* pvData){
	if(kIsQueueFull(pstQueue) == TRUE)
		return FALSE;

	kMemCpy((char*)pstQueue->pvQueueArray+(pstQueue->iDataSize * pstQueue->iPutIndex), pvData, pstQueue->iDataSize);

	pstQueue->iPutIndex = (pstQueue->iPutIndex + 1) % pstQueue->iMaxDataCount;
	pstQueue->bLastOperationPut = TRUE;
	return TRUE;
}

BOOL kGetQueue(QUEUE* pstQueue, void* pvData){
	if(kIsQueueEmpty(pstQueue) == TRUE)
		return FALSE;

	kMemCpy(pvData, (char*)pstQueue->pvQueueArray + (pstQueue->iDataSize * pstQueue->iGetIndex), pstQueue->iDataSize);

	pstQueue->iGetIndex = (pstQueue->iGetIndex + 1) % pstQueue->iMaxDataCount;
	pstQueue->bLastOperationPut = FALSE;
	return TRUE;
}