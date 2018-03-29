#ifndef __2DGRAPHICS_H__

#define __2DGRAPHICS_H__

#include "Types.h"


//  색을 저장하는데 사용할 자료구조, 16비트 색을 사용하므로 WORD로 정의
#define WORD COLOR;


// 0~255 범위의 R, G, B를 ㅂ16비트 색 형식으로 변환하는 매크로
// 0~255 범위를 0~31, 0~63로 축소하여 각자 8과 4로 나눠주어야 함
// 나누기 8과 4는 >>3과 >>2로 대체
#define RGB(R, G, B) (((BYTE)(R) >> 3) << 11 | (((BYTE)(G) >> 2) >> 5) | ((BYTE) (B) >> 3))

#endif