#ifndef __CACHE_MANAGER_H__
#define __CACHE_MANAGER_H__

#include "Types.h"

/***** ��ũ�� ���� *****/
// ĳ�� ���� ���� ��ũ��
#define CACHE_MAXCLUSTERLINKTABLEAREACOUNT 16         // Ŭ������ ��ũ ���̺� ������ �ִ� ĳ�� ���� ����
#define CACHE_MAXDATAAREACOUNT             32         // ������ ������ �ִ� ĳ�� ���� ����
#define CACHE_INVALIDTAG                   0xFFFFFFFF // ��ȿ���� ���� �±�(�� ĳ�� ����)

// ĳ�� ���̺� ���� ��ũ��
#define CACHE_MAXCACHETABLEINDEX           2          // ĳ�� ���̺��� �ִ� �ε��� ����(�ִ� ��Ʈ�� ����)
#define CACHE_CLUSTERLINKTABLEAREA         0          // ĳ�� ���̺��� Ŭ������ ��ũ ���̺� ���� �ε���(ĳ�� ���̺��� 0��° ��Ʈ���� 16���� ĳ�� ���۰� ����)
#define CACHE_DATAAREA                     1          // ĳ�� ���̺��� ������ ���� �ε���(ĳ�� ���̺��� 1��° ��Ʈ���� 32���� ĳ�� ���۰� ����)

/***** ����ü ���� *****/
#pragma pack(push, 1)

typedef struct kCacheBufferStruct{
	DWORD dwTag;        // �±�(ĳ�� ���ۿ� �����Ǵ� Ŭ������ ��ũ ���̺� ����[512B�� ���� ����] �Ǵ� ������ ����[4KB�� Ŭ������ ����]�� ������)
	DWORD dwAccessTime; // ���� �ð�(ĳ�� ���ۿ� ������ �ð�)
	BOOL bChanged;      // ���� ���� �÷���(�����Ͱ� ����Ǿ����� ����)
	BYTE* pbBuffer;     // ������ ����(ĳ�� ������ �ּҰ�)
} CACHEBUFFER;

typedef struct kCacheManagerStruct{
	DWORD vdwAccessTime[CACHE_MAXCACHETABLEINDEX];                                 // ���� �ð�(ĳ�� ���ۺ� ���� �ð��� �ִ밪+1�� �����)
	BYTE* vpbBuffer[CACHE_MAXCACHETABLEINDEX];                                     // ������ ����
	CACHEBUFFER vvstCacheBuffer[CACHE_MAXCACHETABLEINDEX][CACHE_MAXDATAAREACOUNT]; // ĳ�� ����(���� ũ��� 16 < 32 �̹Ƿ� 32�� ����)
	DWORD vdwMaxCount[CACHE_MAXCACHETABLEINDEX];                                   // �ִ� ĳ�� ���� ����
} CACHEMANAGER;

#pragma pack(pop)

/***** �Լ� ���� *****/
BOOL kInitializeCacheManager(void);
CACHEBUFFER* kAllocateCacheBuffer(int iCacheTableIndex);          // �� ĳ�� ���� �˻�
CACHEBUFFER* kFindCacheBuffer(int iCacheTableIndex, DWORD dwTag); // �±�(������)�� ��ġ�ϴ� ĳ�� ���� �˻�
CACHEBUFFER* kGetVictimInCacheBuffer(int iCacheTableIndex);       // �� ĳ�� ���� �Ǵ� ������ ĳ�� ���� �˻�
void kDiscardAllCacheBuffer(int iCacheTableIndex);
BOOL kGetCacheBufferAndCount(int iCacheTableIndex, CACHEBUFFER** ppstCacheBuffer, int* piMaxCount);
static void kCutDownAccessTime(int iCacheTableIndex);

#endif // __CACHE_MANAGER_H__
