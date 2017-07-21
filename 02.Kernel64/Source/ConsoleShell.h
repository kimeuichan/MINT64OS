#ifndef __CONSOLE_SHELL_H__
#define __CONSOLE_SHELL_H__

#include "Types.h"

/***** ﾸￅￅﾩﾷￎ ￁ﾤ￀ￇ *****/
#define CONSOLESHELL_MAXCOMMANDBUFFERCOUNT 300
#define CONSOLESHELL_PROMPTMESSAGE         "MINT64>"


/***** ￅﾸ￀ￔ ￁ﾤ￀ￇ *****/
typedef void (*CommandFunction)(const char* pcParameter);

/***** ﾱﾸ￁ﾶￃﾼ ￁ﾤ￀ￇ *****/
#pragma pack(push, 1)

typedef struct kShellCommandEntryStruct{
	char* pcCommand;
	char* pcHelp;
	CommandFunction pfFunction;
} SHELLCOMMANDENTRY;

typedef struct kParameterListStruct{
	const char* pcBuffer; // ￆￄﾶ￳ﾹￌￅￍ ﾸﾮﾽﾺￆﾮ ﾹ￶ￆￛ
	int iLength;          // ￆￄﾶ￳ﾹￌￅￍ ﾸﾮﾽﾺￆﾮ ﾹ￶ￆￛ ﾱ￦￀ￌ
	int iCurrentPosition; // ￇ￶￀￧ ￆￄﾶ￳ﾹￌￅￍ ￀ﾧￄﾡ
}PARAMETERLIST;

#pragma pack(pop)

/***** ￇￔﾼ￶ ￁ﾤ￀ￇ *****/
// ﾽﾩ ￄￚﾵ￥ ￇￔﾼ￶
void kStartConsoleShell(void);
void kExecuteCommand(const char* pcCommandBuffer);
void kInitializeParameter(PARAMETERLIST* pstList, const char* pcParameterBuffer);
int kGetNextParameter(PARAMETERLIST* pstList, char* pcParameter);

// ￄ﾿ﾸￇﾵ￥ ￃﾳﾸﾮ ￇￔﾼ￶
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
static void kCreateTestTask(const char* pcParameterBuffer);
static void kChangeTaskPriority(const char* pcParameterBuffer);
static void kShowTaskList(const char* pcParameterBuffer);
static void kKillTask(const char* pcParameterBuffer);
static void kCPULoad(const char* pcParameterBuffer);
static void kTestMutex(const char* pcParameterBuffer);
static void kCreateThreadTask(void);
static void kTestThread(const char* pcParameterBuffer);
static void kShowMatrix(const char* pcParameterBuffer);
static void kTestPIE(const char* pcParameterBuffer);
#endif // __CONSOLE_SHELL_H__
