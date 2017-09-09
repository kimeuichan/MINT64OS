#ifndef __CONSOLE_SHELL_H__
#define __CONSOLE_SHELL_H__

#include "Types.h"

/***** ��ũ�� ���� *****/
#define CONSOLESHELL_MAXCOMMANDBUFFERCOUNT 300
#define CONSOLESHELL_PROMPTMESSAGE         "MINT64>"

/***** Ÿ�� ���� *****/
typedef void (*CommandFunction)(const char* pcParameter);

/***** ����ü ���� *****/
#pragma pack(push, 1)

typedef struct kShellCommandEntryStruct{
	char* pcCommand;
	char* pcHelp;
	CommandFunction pfFunction;
} SHELLCOMMANDENTRY;

typedef struct kParameterListStruct{
	const char* pcBuffer; // �Ķ���� ����Ʈ ����
	int iLength;          // �Ķ���� ����Ʈ ���� ����
	int iCurrentPosition; // ���� �Ķ���� ��ġ
}PARAMETERLIST;

#pragma pack(pop)

/***** �Լ� ���� *****/
// �� �ڵ� �Լ�
void kStartConsoleShell(void);
void kExecuteCommand(const char* pcCommandBuffer);
void kInitializeParameter(PARAMETERLIST* pstList, const char* pcParameterBuffer);
int kGetNextParameter(PARAMETERLIST* pstList, char* pcParameter);

// Ŀ�ǵ� ó�� �Լ�
static void kHelp(const char* pcParameterBuffer);
static void kCls(const char* pcParameterBuffer);
static void kShowTotalRAMSize(const char* pcParameterBuffer);
static void kStringToDecimalHexTest(const char* pcParameterBuffer);
static void kShutdown(const char* pcParameterBuffer);
static void kSetTimer(const char* pcParameterBuffer);
static void kWaitUsingPIT(const char* pcParameterBuffer);
static void kReadTimeStampCounter(const char* pcParameterBuffer);
static void kMeasureProcessorSpeed(const char* pcParameterBuffer);
static void kShowDateAndTime(const char* pcParameterBuffer);
static void kTestTask1(void);
static void kTestTask2(void);
static void kCreateTestTask(const char* pcParameterBuffer);
static void kChangeTaskPriority(const char* pcParameterBuffer);
static void kShowTaskList(const char* pcParameterBuffer);
static void kKillTask(const char* pcParameterBuffer);
static void kCPULoad(const char* pcParameterBuffer);
static void kPrintNumberTask(const char* pcParameterBuffer);
static void kTestMutex(const char* pcParameterBuffer);
static void kCreateThreadTask(void);
static void kTestThread(const char* pcParameterBuffer);
QWORD kRandom(void);
static void kDropCharacterThread(void);
static void kMatrixProcess(void);
static void kShowMatrix(const char* pcParameterBuffer);
static void kFPUTestTask(void);
static void kTestPIE(const char* pcParameterBuffer);
static void kShowDynamicMemoryInformation(const char* pcParameterBuffer);
static void kTestSequentialAllocation(const char* pcParameterBuffer);
static void kRandomAllocationTask(void);
static void kTestRandomAllocation(const char* pcParameterBuffer);
static void kShowHDDInformation(const char* pcParameterBuffer);
static void kReadSector(const char* pcParameterBuffer);
static void kWriteSector(const char* pcParameterBuffer);
static void kMountHDD(const char* pcParameterBuffer);
static void kFormatHDD(const char* pcParameterBuffer);
static void kShowFileSystemInformation(const char* pcParameterBuffer);
static void kCreateFileInRootDirectory(const char* pcParameterBuffer);
static void kDeleteFileInRootDirectory(const char* pcParameterBuffer);
static void kShowRootDirectory(const char* pcParameterBuffer);
static void kWriteDataToFile(const char* pcParameterBuffer);
static void kReadDataFromFile(const char* pcParameterBuffer);
static void kTestFileIO(const char* pcParameterBuffer);
static void kTestPerformance(const char* pcParameterBuffer);
static void kFlushCache(const char* pcParameterBuffer);
static void kDownloadFile(const char* pcParameterBuffer);
#endif // __CONSOLE_SHELL_H__
