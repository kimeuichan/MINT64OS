#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>



#define MY_O_BINARY     0x10000 // O_BINARY가 이클립스상에서 심볼에러가 발생하여, 임시방편으로 매크로 변수 재정의.
#define BYTES_OF_SECTOR 512

int copyFile(int iSourceFd, int iTargetFd);
int adjustInSectorSize(int iFd, int iSourceSize);
void writeKernelInformation(int iTargetFd, int iTotalSectorCount, int iKernel32SectorCount);

int main(int argc, const char** argv){
	int iSourceFd;
	int iTargetFd;
	int iBootLoaderSectorCount;
	int iKernel32SectorCount;
	int iKernel64SectorCount;
	int iSourceSize;

	// 파라미터 수 체크
	if(argc != 5){
		fprintf(stderr, "[ERROR] Usage: ImageMaker.exe <TargetFile> <SourceFile1> <SourceFile2> <SourceFile3>\n");
		exit(-1);
	}

	const char* pcTargetFile  = argv[1];
	const char* pcSourceFile1 = argv[2];
	const char* pcSourceFile2 = argv[3];
	const char* pcSourceFile3 = argv[4];

	// <TargetFile> 오픈
	if((iTargetFd = open(pcTargetFile, O_RDWR | O_CREAT | O_TRUNC | MY_O_BINARY | S_IREAD | S_IWRITE)) == -1){
		fprintf(stderr, "[ERROR] Disk.img open fail.\n");
		exit(-1);
	}

	// <SourceFile1> 오픈, 복사, 크기 조정
	printf("[INFO] Copy %s to %s\n", pcSourceFile1, pcTargetFile);
	if((iSourceFd = open(pcSourceFile1, O_RDONLY | MY_O_BINARY)) == -1){
		fprintf(stderr, "[ERROR] %s open fail.\n", pcSourceFile1);
		exit(-1);
	}

	iSourceSize = copyFile(iSourceFd, iTargetFd);
	close(iSourceFd);

	iBootLoaderSectorCount = adjustInSectorSize(iTargetFd, iSourceSize);
	printf("[INFO] %s: file_size=[%d], sector_count=[%d]\n", pcSourceFile1, iSourceSize, iBootLoaderSectorCount);

	// <SourceFile2> 오픈, 복사, 크기 조정
	printf("[INFO] Copy %s to %s\n", pcSourceFile2, pcTargetFile);
	if((iSourceFd = open(pcSourceFile2, O_RDONLY | MY_O_BINARY)) == -1){
		fprintf(stderr, "[ERROR] %s open fail.\n", pcSourceFile2);
		exit(-1);
	}

	iSourceSize = copyFile(iSourceFd, iTargetFd);
	close(iSourceFd);

	iKernel32SectorCount = adjustInSectorSize(iTargetFd, iSourceSize);
	printf("[INFO] %s: file_size=[%d], sector_count=[%d]\n", pcSourceFile2, iSourceSize, iKernel32SectorCount);

	// <SourceFile3> 오픈, 복사, 크기 조정
	printf("[INFO] Copy %s to %s\n", pcSourceFile3, pcTargetFile);
	if((iSourceFd = open(pcSourceFile3, O_RDONLY | MY_O_BINARY)) == -1){
		fprintf(stderr, "[ERROR] %s open fail.\n", pcSourceFile3);
		exit(-1);
	}

	iSourceSize = copyFile(iSourceFd, iTargetFd);
	close(iSourceFd);

	iKernel64SectorCount = adjustInSectorSize(iTargetFd, iSourceSize);
	printf("[INFO] %s: file_size=[%d], sector_count=[%d]\n", pcSourceFile3, iSourceSize, iKernel64SectorCount);

	// <TargetFile>에 섹터 수 쓰기
	printf("[INFO] Start to write kernel information.\n");
	writeKernelInformation(iTargetFd, iKernel32SectorCount + iKernel64SectorCount, iKernel32SectorCount);
	printf("[INFO] %s create complete.\n", pcTargetFile);

	close(iTargetFd);

	return 0;
}

int copyFile(int iSourceFd, int iTargetFd){
	int iSourceSize = 0;
	int iReadSize;
	int iWriteSize;
	char vcBuffer[BYTES_OF_SECTOR];

	while(1){
		iReadSize = read(iSourceFd, vcBuffer, sizeof(vcBuffer));
		iWriteSize = write(iTargetFd, vcBuffer, iReadSize);

		if(iReadSize != iWriteSize){
			fprintf(stderr, "[ERROR] Read size is not equal to write size.\n");
			exit(-1);
		}

		iSourceSize += iReadSize;

		if(iReadSize != sizeof(vcBuffer)){
			break;
		}
	}

	return iSourceSize;
}

int adjustInSectorSize(int iFd, int iSourceSize){
	int i;
	int iAjustSize;
	char cZero;
	int iSectorCount;

	iAjustSize = iSourceSize % BYTES_OF_SECTOR;
	cZero = 0x00;

	if(iAjustSize != 0){
		iAjustSize = BYTES_OF_SECTOR - iAjustSize;
		printf("[INFO] Adjust Size(YES): file size=[%lu], fill size=[%u]\n", iSourceSize, iAjustSize);

		for(i = 0; i < iAjustSize; i++){
			write(iFd, &cZero, 1);
		}

	}else{
		printf("[INFO] Adjust Size(NO): File size(%lu) is already aligned with a sector size.\n", iSourceSize);
	}

	iSectorCount = (iSourceSize + iAjustSize) / BYTES_OF_SECTOR;

	return iSectorCount;
}

void writeKernelInformation(int iTargetFd, int iTotalSectorCount, int iKernel32SectorCount){
	unsigned short usData;
	long lPosition;

	lPosition = lseek(iTargetFd, 5, SEEK_SET);
	if(lPosition == -1){
		fprintf(stderr, "[ERROR] lseek fail. file_pointer=[%d], errno=[%d], lseek_option=[%d]\n", lPosition, errno, SEEK_SET);
		exit(-1);
	}

	usData = (unsigned short)iTotalSectorCount;
	write(iTargetFd, &usData, 2);
	usData = (unsigned short)iKernel32SectorCount;
	write(iTargetFd, &usData, 2);

	printf("[INFO] TOTAL_SECTOR_COUNT(except BootLoader)=[%d]\n", iTotalSectorCount);
	printf("[INFO] KERNEL32_SECTOR_COUNT=[%d]\n", iKernel32SectorCount);
}
