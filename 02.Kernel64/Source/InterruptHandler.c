#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "HardDisk.h"

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode){
	char vcBuffer[3] = {0, };

	// ���� ��ȣ ���� (2�ڸ� ����)
	vcBuffer[0] = '0' + iVectorNumber / 10;
	vcBuffer[1] = '0' + iVectorNumber % 10;

	kPrintStringXY(0, 0, "==================================================");
	kPrintStringXY(0, 1, "               Exception Occur~!!                 ");
	kPrintStringXY(0, 2, "                  Vector: [  ]                    ");
	kPrintStringXY(27, 2, vcBuffer); // "Vector:" ���ڿ� ���� ���
	kPrintStringXY(0, 3, "==================================================");

	while(1);
}

void kCommonInterruptHandler(int iVectorNumber){
	char vcBuffer[] = "[INT:  , ]";
	static int g_iCommonInterruptCount = 0;

	//====================================================================================================
	// ���ͷ�Ʈ�� �߻������� �˸����� �޼����� ����ϴ� �κ�
	// ���� ��ȣ ���� (2�ڸ� ����)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// �߻� Ƚ�� ���� (1�ڸ� ����)
	g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) % 10;
	vcBuffer[8] = '0' + g_iCommonInterruptCount;

	// ù��° ���� ������ ��ġ�� ���
	kPrintStringXY(70, 0, vcBuffer);
	//====================================================================================================

	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

void kKeyboardHandler(int iVectorNumber){
	char vcBuffer[] = "[INT:  , ]";
	static int g_iKeyboardInterruptCount = 0;
	BYTE bScanCode;

	//====================================================================================================
	// ���ͷ�Ʈ�� �߻������� �˸����� �޼����� ����ϴ� �κ�
	// ���� ��ȣ ���� (2�ڸ� ����)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// �߻� Ƚ�� ���� (1�ڸ� ����)
	g_iKeyboardInterruptCount = (g_iKeyboardInterruptCount + 1) % 10;
	vcBuffer[8] = '0' + g_iKeyboardInterruptCount;

	// ù��° ���� ù��° ��ġ�� ���
	kPrintStringXY(0, 0, vcBuffer);
	//====================================================================================================

	if(kIsOutputBufferFull() == TRUE){
		bScanCode = kGetKeyboardScanCode();
		kConvertScanCodeAndPutQueue(bScanCode);
	}

	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

void kTimerHandler(int iVectorNumber){
	char vcBuffer[] = "[INT:  , ]";
	static int g_iTimerInterruptCount = 0;

	//====================================================================================================
	// ���ͷ�Ʈ�� �߻������� �˸����� �޼����� ����ϴ� �κ�
	// ���� ��ȣ ���� (2�ڸ� ����)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// �߻� Ƚ�� ���� (1�ڸ� ����)
	g_iTimerInterruptCount = (g_iTimerInterruptCount + 1) % 10;
	vcBuffer[8] = '0' + g_iTimerInterruptCount;

	// ù��° ���� ������ ��ġ�� ���
	kPrintStringXY(70, 0, vcBuffer);
	//====================================================================================================

	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);

	g_qwTickCount++;

	kDecreaseProcessorTime();

	if(kIsProcessorTimeExpired() == TRUE){
		kScheduleInInterrupt();
	}
}

void kDeviceNotAvailableHandler(int iVectorNumber){
	TCB* pstFPUTask;       // ������ FPU ��� �½�ũ
	TCB* pstCurrentTask;   // ���� �½�ũ
	QWORD qwLastFPUTaskID; // ������ FPU ��� �½�ũ ID

	//====================================================================================================
	// FPU ���ܰ� �߻������� �˸����� �޼����� ����ϴ� �κ�
	char vcBuffer[] = "[EXC:  , ]";
	static int g_iFPUExceptionCount = 0;

	// ���� ��ȣ ���� (2�ڸ� ����)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// �߻� Ƚ�� ���� (1�ڸ� ����)
	g_iFPUExceptionCount = (g_iFPUExceptionCount + 1) % 10;
	vcBuffer[8] = '0' + g_iFPUExceptionCount;

	// ù��° ���� ù��° ��ġ�� ���
	kPrintStringXY(0, 0, vcBuffer);
	//====================================================================================================

	// CR0.TS=0 ���� ����
	kClearTS();

	qwLastFPUTaskID = kGetLastFPUUsedTaskID();
	pstCurrentTask = kGetRunningTask();

	// ������ FPU ��� �½�ũ�� ���� �½�ũ�� ���, FPU ���ؽ�Ʈ�� ���� �� ������ �ʿ� ����, ���� FPU ���ؽ�Ʈ�� �״�� ����ϸ� �ǹǷ� �Լ� ����
	if(qwLastFPUTaskID == pstCurrentTask->stLink.qwID){
		return;

	// ������ FPU ��� �½�ũ�� ���� �ϴ� ���, ���� FPU ���ؽ�Ʈ�� �޸𸮿� ����
	}else if(qwLastFPUTaskID != TASK_INVALIDID){
		pstFPUTask = kGetTCBInTCBPool(GETTCBOFFSET(qwLastFPUTaskID));
		if((pstFPUTask != NULL) && (pstFPUTask->stLink.qwID == qwLastFPUTaskID)){
			kSaveFPUContext(pstFPUTask->vqwFPUContext);
		}
	}

	// ���� �½�ũ�� FPU�� ����� ���� ���� ���, FPU �ʱ�ȭ
	if(pstCurrentTask->bFPUUsed == FALSE){
		kInitializeFPU();
		pstCurrentTask->bFPUUsed = TRUE;

	// ���� �½�ũ�� FPU�� ����� ���� �ִ� ���, ���� �½�ũ�� FPU ���ؽ�Ʈ�� �޸𸮿��� ����
	}else{
		kLoadFPUContext(pstCurrentTask->vqwFPUContext);
	}

	// ������ FPU ��� �½�ũ�� ���� �½�ũ�� ����
	kSetLastFPUUsedTaskID(pstCurrentTask->stLink.qwID);
}

void kHDDHandler(int iVectorNumber){
	char vcBuffer[] = "[INT:  , ]";
	static int g_iHDDInterruptCount = 0;

	//====================================================================================================
	// ���ͷ�Ʈ�� �߻������� �˸����� �޼����� ����ϴ� �κ�
	// ���� ��ȣ ���� (2�ڸ� ����)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// �߻� Ƚ�� ���� (1�ڸ� ����)
	g_iHDDInterruptCount = (g_iHDDInterruptCount + 1) % 10;
	vcBuffer[8] = '0' + g_iHDDInterruptCount;

	// ù��° ���� �ι�° ��ġ�� ���
	kPrintStringXY(10, 0, vcBuffer);
	//====================================================================================================

	// ù��° PATA ��Ʈ�� ���ͷ�Ʈ ����(IRQ14) ó��
	if((iVectorNumber - PIC_IRQSTARTVECTOR) == 14){
		// ù��° PATA ��Ʈ��  ���ͷ�Ʈ �÷��׸� TRUE�� ����
		kSetHDDInterruptFlag(TRUE, TRUE);

	// ù��° PATA ��Ʈ�� ���ͷ�Ʈ ����(IRQ15) ó��
	}else{
		// �ι�° PATA ��Ʈ��  ���ͷ�Ʈ �÷��׸� TRUE�� ����
		kSetHDDInterruptFlag(FALSE, TRUE);
	}

	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}
