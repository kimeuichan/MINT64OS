#ifndef __ASSEMBLY_UTILITY_H_
#define __ASSEMBLY_UTILITY_H_

#include "Types.h"

BYTE kInPortByte(WORD wPort);
void kOutPortByte(WORD wPort, BYTE bData);

#endif
