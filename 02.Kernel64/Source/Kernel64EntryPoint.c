#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"
#include "Task.h"
#include "PIT.h"
#include "Utility.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"
#include "MultiProcessor.h"
#include "VBE.h"

// AP 를 위한 메인
void MainForApplicationProcessor(void);
// 그래픽 모드를 테스트하는 함수
void kStartGraphicModeTest();

// Bootstrap Processor C 언어 커널 엔트리 포인트
// 아래 함수는 C 언어 커널의 시작 부분임

void Main(void){
	int iCursorX, iCursorY;

	// 부트 로더에 bsp 플래그 읽어서 ap 이면
	// ap 용 main 으로 이동
	if(*((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) == 0)
		MainForApplicationProcessor();

	// bsp 부팅 완료 했으므로, 0x7c09 에 있는 bsp 플래그를
	// 0설정하여 ap용으로 코드 경로 변경
	*((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) = 0;

	// 콘솔을 먼저 초기화 한 후 다음 작업을 수행
	kInitializeConsole(0, 10);

	// IA-32e 모드 C언어 커널 시작 메세지
	kPrintf("Switch to IA-32e Mode Success~!!\n");
	kPrintf("IA-32e Mode C Language Kernel Start.........[Pass]\n");
	kPrintf("Console Initialize..........................[Pass]\n");

	// GDT/TSS 생성 및 GDT 로드
	kGetCursor(&iCursorX, &iCursorY);
	kPrintf("GDT/TSS Initialize and GDT Load.............[    ]");
	kInitializeGDTTableAndTSS();
	kLoadGDTR(GDTR_STARTADDRESS);
	kSetCursor(45, iCursorY++);
	kPrintf("Pass\n");

	// TSS 로드
	kPrintf("TSS Load....................................[    ]");
	kLoadTR(GDT_TSSSEGMENT);
	kSetCursor(45, iCursorY++);
	kPrintf("Pass\n");

	// IDT 생성 및 로드
	kPrintf("IDT Initialize and Load.....................[    ]");
	kInitializeIDTTable();
	kLoadIDTR(IDTR_STARTADDRESS);
	kSetCursor(45, iCursorY++);
	kPrintf("Pass\n");

	// 총 RAM 크기 체크
	kPrintf("Total RAM Size Check........................[    ]");
	kCheckTotalRAMSize();
	kSetCursor(45, iCursorY++);
	kPrintf("Pass], Size = %d MB\n", kGetTotalRAMSize());

	// TCB 풀 및 스케줄러 초기화
	kPrintf("TCB Pool and Scheduler Initialize...........[Pass]\n");
	iCursorY++;
	kInitializeScheduler();

	// 동적 메모리 초기화
	kPrintf("Dynamic Memory Initialize...................[Pass]\n");
	iCursorY++;
	kInitializeDynamicMemory();

	// PIT 초기화
	kInitializePIT(MSTOCOUNT(1), TRUE); // 1ms당 한 번씩(주기적으로) 타이머 인터럽트가 발생하도록 설정

	// 키 큐 초기화 및 키보드 활성화
	kPrintf("Key-Queue Initialize and Keyboard Activate..[    ]");
	if(kInitializeKeyboard() == TRUE){
		kSetCursor(45, iCursorY++);
		kPrintf("Pass\n");
		kChangeKeyboardLED(FALSE, FALSE, FALSE);

	}else{
		kSetCursor(45, iCursorY++);
		kPrintf("Fail\n");
		while(1);
	}

	// PIC 초기화 및 인터럽트 활성화
	kPrintf("PIC Initialize and Interrupt Activate.......[    ]");
	kInitializePIC();
	kMaskPICInterrupt(0);
	kEnableInterrupt();
	kSetCursor(45, iCursorY++);
	kPrintf("Pass\n");

	// 파일 시스템 초기화
	kPrintf("File System Initialize......................[    ]");
	if(kInitializeFileSystem() == TRUE){
		kSetCursor(45, iCursorY++);
		kPrintf("Pass\n");

	}else{
		kSetCursor(45, iCursorY++);
		kPrintf("Fail\n");
	}

	// 시리얼 포트 초기화
	kPrintf("Serial Port Initialize......................[Pass]\n");
	iCursorY++;
	kInitializeSerialPort();

	// 유휴 태스크를 생성하고, 콘솔 쉘을 시작
	kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0, (QWORD)kIdleTask, kGetAPICID());

	// 그래픽 모드가 아니면 콘솔 셸 실행
	if( *(BYTE*) VBE_STARTGRAPHICMODEFLAGADDRESS == 0)
		kStartConsoleShell();
	else
		kStartGraphicModeTest();
}

// AP 용 Main
// 대부분의 자료구조는 bsp 가 생성했으므로 코어에 설정 하는 작업만 함
void MainForApplicationProcessor(void){
	QWORD qwTickCount;

	// GDT 테이블을 설정
	kLoadGDTR(GDTR_STARTADDRESS);

	// TSS 디스크립터를 설정. TSS 세그먼트와 디스크립터를 ap의
	// 수만큼 생성했으므로, APIC ID를 이용하여 TSS 디스크립터를 할당
	kLoadTR(GDT_TSSSEGMENT + (kGetAPICID()*sizeof(GDTENTRY16)));

	// IDT 테이블 설정
	kLoadIDTR(IDTR_STARTADDRESS);

	// 스케줄러 초기화
	kInitializeScheduler();

	// 현재 코어의 로컬 APIC를 활성화
    kEnableSoftwareLocalAPIC();

    // 모든 인터럽트를 수신할 수 있도록 태스크 우선 순위 레지스터를 0으로 설정
    kSetTaskPriority( 0 );

    // 로컬 APIC의 로컬 벡터 테이블을 초기화
    kInitializeLocalVectorTable();

    // 인터럽트를 활성화
    kEnableInterrupt();  

	kPrintf("Application Processor[APIC ID: %d] is Activated\n", kGetAPICID());
	
	// 유휴 태스크 실행
	kIdleTask();
}

// 그래픽 모드를 테스트하는 함수
void kStartGraphicModeTest(){
	VBEMODEINFOBLOCK* pstVBEMode;
	WORD* pwFrameBufferAddress;
	WORD wColor = 0;
	int iBandHeight;
	int i,j;

	// 키 입력 대기
	kGetCh();

	// VBE 모드 정보 블록을 반화하고 선형 프레임 버퍼의 시작 어드레스를 저장
	pstVBEMode = kGetVBEModeInfoBlock();
	pwFrameBufferAddress = (WORD*)( (QWORD)pstVBEMode->dwPhysicalBasePointer);

	// 화면을 세로로 32 등분하여 색을 칠함
	iBandHeight = pstVBEMode->wYResolution / 32;

	while(1){
		for(j=0; j<pstVBEMode->wYResolution; j++){

			// X 축의 크기 만큼 프레임 버퍼에 색을 저장
			for(i=0; i<pstVBEMode->wXResolution; i++){
				// 비디오 메모리 오프셋을 계산하는 부분
				// Y축의 현재 위치(j)에 X축의 크기를 곱하면 Y축의 어드레스를
				// 계산할 수 있고, 여기에 X 축의 오프셋(i)을 더하면 현재 픽셀을 출력할 어드레스를 구할 수 있음
				pwFrameBufferAddress[(j * pstVBEMode->wXResolution)+i] = wColor;

			}

			// Y 위치가 32 등분한 단위로 나누어 떨어지면 색을 바꿈
			if((j % iBandHeight) == 0)
				wColor = kRandom() % 0xffff;
		}
		kGetCh();

	}

          
}
