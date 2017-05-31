#include "Types.h"
#include "AssemblyUtility.h"
#include "Keybord.h"

/////////////////////////////////////////////////////////
// 키보드 컨트롤러와 키보드 제어에 관련된 함수
/////////////////////////////////////////////////////////


// 출력 버퍼(0x60)에 수신된 데이터가 있는지 여부 반환
BOOL kIsOutputBufferFull(void){
	// 상태 레지스터(0x64)에서 읽은 값에 출력 버퍼 상태 비트가
	// 1로 설정되어 있으면 출력 버퍼에 키보드가 전송한 데이터가 존재
	if(kInPortByte(0x64) & 0x01){
		return TRUE;
	}
	return FALSE;
}

// 입력 버퍼에 프로세서가 쓴 데이터 있는지 여부 반환
BOOL kIsInputBufferFull(void){
	// 상태 레지스터(0x64)에서 읽은 값에 입력 버퍼 상태 비트가
	// 1로 설정되어 있으면 키보드가 데이터를 가져가지 않았음
	if(kInPortByte(0x64) & 0x02)
		return TRUE;

	return FALSE;
}

// 키보드를 활성화
BOOL kActivateKeyboard(void){
	int i;
	int j;

	// 컨트롤 레지스터에 키보드 활성화 커맨드를 전달하여 키보드 디바이스 활성화
	kOutPortByte(0x64, 0xAE);

	// 입력버퍼가 빌 때까지 기다렸다가 키보드에 활성화 커맨드 전송
	// 0xffff 만큼 루프를 수행할 시간이면 충분히 커맨드가 전송될 수 있음
	// 0xffff 루프를 수행한 이후에도 입력버퍼가 비지 않으면 무시하고 전송
	for(i=0; i<0xffff; i++){
		// 입력버퍼가 비어있으면 키보드 커맨드 전송 가능
		if(kIsOutputBufferFull == FALSE)
			break;
	}

	// 입력 버퍼로 키보드 활성화 커맨드를 전달하여 키보드로 전송
	kOutPortByte(0x60, 0xf4);

}