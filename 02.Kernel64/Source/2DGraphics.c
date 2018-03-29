#include "2DGraphics.h"

inline void kDrawPixel(int iX, int iY, COLOR stColor){
	VBEMODEINFOBLOCK* pstModeInfo;

	pstModeInfo = kGetVBEModeInfoBlock();

	*((COLOR*)(QWORD)pstModeInfo->dwPhysicalBasePointer) + pstModeInfo->wXResolution * iY + iX) = stColor;
}

void kDrawLine(int iX1, iY1, int iX2, int iY2, COLOR stColor){
	int iDeltaX, iDeltaY;
	int iError = 0;
	int iDeltaError;
	int iX, iY;
	int iStepX, iStepY;

	// 변화량 계산
	iDeltaX = iX2 - iX1;
	iDeltaY = iY2 - iY1;

	// X 축 변화량에 따라 X축 증감 방향 계산
	if(iDeltaX < 0){
		iDeltaX = -iDeltaX;
		iStepX = -1;
	}
	else
		iStepX = 1;


	// Y 축 변화량에 따라 Y축 증감 방향 계산
	if(iDeltaY < 0){
		iDeltaY = -iDeltaY;
		iStepY = -1;
	}
	else
		iStepY = 1;

	// X축 변화량이 Y축 변화량보다 크다면 X축을 중심으로 직서능ㄹ 그림
	if(iDeltaX > iDeltaY){
		// 기울기로 픽세람다 더해줄 오차, Y축 변화량의 2배
		// 시프트 연산으로 *2를 대체
		iDeltaError = iDeltaY << 1;
		iY = iY1;
		for(iX=iX1; iX1 != iX2; iX += iStepX){
			// 점 그리기
			kDrawPixel(iX, iY, stColor)

			// 오류 누적
			iError += iDeltaError;

			// 누적된 오차가 X축 변화량 보다 크면 위에 점을 선택하고 오차를 위에 점을 기준으로 갱신
			if( iError >= iDeltaX){
				iY += iStepY;
				// X축의 변화량이 2배를 빼줌
				// 시프트 연산으로 *2를 대체
				iError -= iDeltaX << 1;
			}

		}
		// iX == iX2인 최종 위치에 점 그리기
		kDrawPixel(iX, iY, stColor);
	}
	// Y축 변화량이 X축 변화량보다 크거나 같다면 Y축을 중심으로 직선을 그림
	else{
		// 기울기로 픽셀마다 더해줄 오차, X축 변화량의 2배
		// 시프트 연산으로 *2를 대체
		iDeltaError = iDeltaX << 1;
		iX = iX1;
		for(iY=iY1; iY!=iY2; iY+=iStepY){
			// 점 그리기
			kDrawPixel(iX, iY, stColor)

			// 오류 누적
			iError += iDeltaError;

			// 누적된 오차가 X축 변화량 보다 크면 위에 점을 선택하고 오차를 위에 점을 기준으로 갱신
			if( iError >= iDeltaY){
				iX += iStepX;
				// X축의 변화량이 2배를 빼줌
				// 시프트 연산으로 *2를 대체
				iError -= iDeltaY << 1;
			}

		}
		// iY == iY2인 최종 위치에 점 그리기
		kDrawPixel(iX, iY, stColor);
	}
}

void kDrawRect(int iX1, int iY1, int iX2, int iY2, COLOR stColor, BOOL bFill){
	int iWidth;
	int iTemp;
	int iY;
	int iStepY;
	COLOR* pstVideoMemoryAddress;
	VBEMODEINFOBLOCK* pstModeInfo;

	// 채움 여부에 따라 코드를 분리
	if(bFill == FALSE){
		// 네 점을 이웃한 것끼리 직선으로 연결
		kDrawLine(iX1, iY1, iX2, iY1, stColor);
		kDrawLine(iX1, iY1, iX1, iY2, stColor);
		kDrawLine(iX2, iY1, iX1, iY1, stColor);
		kDrawLine(iX2, iY2, iX1, iY2, stColor);
	}
	else{
		pstModeInfo = kGetVBEModeInfoBlock();

		// 비디오 메모리 어드레스 계산
		pstVideoMemoryAddress = (COLOR*)((QWORD)pstModeInfo->dwPhysicalBasePointer);

		// kMemSetWord() 함수는 X의 값이 증가하는 방향으로 데이터를 채우므로
		// x1과 x2를 비교하여 두점을 교환
		if(iX2 < iX1){
			iTemp = iX1;
			iX1 = iX2;
			iX2 = iTemp;

			iTemp = iY1;
			iY1 = iY2;
			iY2 = iTemp;
		}

		// 사각형의 X축 길이 게산
		iWidth = iX2 - iX1 + 1;

		// y1과 y2를 비교하여 매 회 증가 시킬 Y값 저장
		if(iY1 <= iY2)
			iStepY = 1;
		else
			iStepY = -1;

		//출력을 시작할 비디오 메모리 어드레스 계산
		pstVideoMemoryAddress += iY1 * pstModeInfo->wXResolution + iX1;

		// 루프를 돌면서 각 Y 축마다 값을 채움
		for(iY=iY1; iY != iY2; iY += iStepY){
			// 비디오 메모리에 사각형의 너비만큼 출력
			kMemSetWord(pstVideoMemoryAddress, stColor, iWidth);

			// 출력할 비디오 메모리 어드레스 갱신
			// X, Y좌표로 매번 비디오 메모리 어드레스를 계산하는 것을 피하려고
			// X 축 해상도를 이용하여 다음 Y의 어드레스를 계산

			// pstVideoMemoryAddress += iStepY * pstModeInfo->wXResolution;

			if(iStepY >= 0)
				pstVideoMemoryAddress += pstModeInfo->wXResolution;
			else
				pstVideoMemoryAddress -= pstModeInfo->wXResolution;
		}
		// 비디오 메모리에 사각형의 너비만큼 출력, 마지막 줄 출력
		kMemSetWord(pstVideoMemoryAddress, stColor, iWidth);
	}
}

void kDrawCircle(int iX, int iY, int iRadius, COLOR stColor, BOOL bFill){
	int iCircleX, iCircleY;
	int iDistance;

	// (0,R) 인 좌표에서 시작
	iCircleY = iRadius;

	// 채움 여부에 따라 시작점을 그림
	if(bFill == FALSE){
		// 시작점은 네 접점 모두 그림
		kDrawPixel(0 + iX, iRadius + iY, stColor);
		kDrawPixel(0 + iX, -iRadius + iY, stColor);
		kDrawPixel(iRadius + iX, 0 + iY, stColor);
		kDrawPixel(-iRadius + iX, 0 + iY, stColor);
	}
	else{
		// 시작 직선은 X축과 Y축 모두 그림
		kDrawLine(0+iX, iRadius + iY, 0+iX, -iRadius + iY, stColor);
		kDrawLine(iRadius + iX, 0 + iY, -iRadius + iX, 0 + iY, stColor);
	}

	// 최초 시작점의 중심점과 원의 거리
	iDistance = -iRadius;

	// 원 그리기
	for(iCircleX=1; iCircleX<= iCircleY; iCircleX++){
		// 원에서 떨어진 거리 계산
		// 시프트 연산으로 *2를 대체
		iDistance += (iCircleX << 1) -1; // 2*iCircleX -1;

		// 중심점이 원의 외부에 있으면 아래에 있는 점을 선택
		if(iDistance >= 0){
			iCircleY--;

			// 새로운 점에서 다시 원과 거리 계산
			// 시프트 연산으로 *2를 대체
			iDistance += (-iCircleY << 1) +2; //-2 * iCircleY +2;
		}

		// 채움 여부에 따라 그림
		if(bFill == FALSE){
			// 8방향 모두 점 그림
			kDrawPixel(iCircleX + iX, iCircleY + iY, stColor);
			kDrawPixel(iCircleX + iX, -iCircleY + iY, stColor);
			kDrawPixel(-iCircleX + iX, iCircleY + iY, stColor);
			kDrawPixel(-iCircleX + iX, -iCircleY + iY, stColor);
			kDrawPixel(iCircleY + iX, iCircleX + iY, stColor);
			kDrawPixel(iCircleY + iX, -iCircleX + iY, stColor);
			kDrawPixel(-iCircleY + iX, iCircleX + iY, stColor);
			kDrawPixel(-iCircleY + iX, -iCircleX + iY, stColor);
		}
		else{
			// 대칭 되는 점을 찾아 X축에 평행한 직선을 그어 채워진 원을 그림
			// 평행선을 그리는 것은 사각형 그리기 함수로 빠르게 처리할 수 있음
			kDrawRect(-iCircleX + iX, iCircleY + iY, iCircleX + iX, iCircleY + iY, stColor, TRUE);
			kDrawRect(-iCircleX + iX, -iCircleY + iY, iCircleX + iX, -iCircleY + iY, stColor, TRUE);
			kDrawRect(-iCircleY + iX, iCircleX + iY, iCircleY + iX, iCircleX + iY, stColor, TRUE);
			kDrawRect(-iCircleY + iX, -iCircleX + iY, iCircleY + iX, -iCircleX + iY, stColor, TRUE);

		}
	}



}