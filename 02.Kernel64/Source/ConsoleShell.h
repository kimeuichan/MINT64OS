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
void kHelp(const char* pcParameterBuffer);
void kCls(const char* pcParameterBuffer);
void kShowTotalRAMSize(const char* pcParameterBuffer);
void kStringToDecimalHexTest(const char* pcParameterBuffer);
void kShutdown(const char* pcParameterBuffer);
void kSetTimer(const char* pcParameterBuffer);
void kWaitUsingPIT(const char* pcParameterBuffer);
void kReadTimeStampCounter(const char* pcParameterBuffer);
void kMeasureProcessorSpeed(const char* pcParameterBuffer);
void kShowDateAndTime(const char* pcParameterBuffer);
void kCreateTestTask(const char* pcParameterBuffer);

#endif // __CONSOLE_SHELL_H__
