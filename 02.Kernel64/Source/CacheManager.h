#ifndef __CACHE_MANAGER_H__
#define __CACHE_MANAGER_H__

#include "Types.h"

/***** 매크로 정의 *****/
// 캐시 버퍼 관련 매크로
#define CACHE_MAXCLUSTERLINKTABLEAREACOUNT 16         // 클러스터 링크 테이블 영역의 최대 캐시 버퍼 개수
#define CACHE_MAXDATAAREACOUNT             32         // 데이터 영역의 최대 캐시 버퍼 개수
#define CACHE_INVALIDTAG                   0xFFFFFFFF // 유효하지 않은 태그(빈 캐시 버퍼)

// 캐시 테이블 관련 매크로
#define CACHE_MAXCACHETABLEINDEX           2          // 캐시 테이블의 최대 인덱스 개수(최대 엔트리 개수)
#define CACHE_CLUSTERLINKTABLEAREA         0          // 캐시 테이블의 클러스터 링크 테이블 영역 인덱스(캐시 테이블의 0번째 엔트리에 16개의 캐시 버퍼가 존재)
#define CACHE_DATAAREA                     1          // 캐시 테이블의 데이터 영역 인덱스(캐시 테이블의 1번째 엔트리에 32개의 캐시 버퍼가 존재)

/***** 구조체 정의 *****/
#pragma pack(push, 1)

typedef struct kCacheBufferStruct{
	DWORD dwTag;        // 태그(캐시 버퍼에 대응되는 클러스터 링크 테이블 영역[512B의 섹터 단위] 또는 데이터 영역[4KB의 클러스터 단위]의 오프셋)
	DWORD dwAccessTime; // 접근 시간(캐시 버퍼에 접근한 시간)
	BOOL bChanged;      // 변경 여부 플래그(데이터가 변경되었는지 여부)
	BYTE* pbBuffer;     // 데이터 버퍼(캐시 버퍼의 주소값)
} CACHEBUFFER;

typedef struct kCacheManagerStruct{
	DWORD vdwAccessTime[CACHE_MAXCACHETABLEINDEX];                                 // 접근 시간(캐시 버퍼별 접근 시간의 최대값+1이 저장됨)
	BYTE* vpbBuffer[CACHE_MAXCACHETABLEINDEX];                                     // 데이터 버퍼
	CACHEBUFFER vvstCacheBuffer[CACHE_MAXCACHETABLEINDEX][CACHE_MAXDATAAREACOUNT]; // 캐시 버퍼(열의 크기로 16 < 32 이므로 32를 선택)
	DWORD vdwMaxCount[CACHE_MAXCACHETABLEINDEX];                                   // 최대 캐시 버퍼 개수
} CACHEMANAGER;

#pragma pack(pop)

/***** 함수 정의 *****/
BOOL kInitializeCacheManager(void);
CACHEBUFFER* kAllocateCacheBuffer(int iCacheTableIndex);          // 빈 캐시 버퍼 검색
CACHEBUFFER* kFindCacheBuffer(int iCacheTableIndex, DWORD dwTag); // 태그(오프셋)가 일치하는 캐시 버퍼 검색
CACHEBUFFER* kGetVictimInCacheBuffer(int iCacheTableIndex);       // 빈 캐시 버퍼 또는 오래된 캐시 버퍼 검색
void kDiscardAllCacheBuffer(int iCacheTableIndex);
BOOL kGetCacheBufferAndCount(int iCacheTableIndex, CACHEBUFFER** ppstCacheBuffer, int* piMaxCount);
static void kCutDownAccessTime(int iCacheTableIndex);

#endif // __CACHE_MANAGER_H__
