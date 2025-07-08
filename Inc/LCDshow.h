#ifndef __QDTFT_DEMO_H
#define __QDTFT_DEMO_H 

#include "function.h"

extern uint32_t ShowCnt;

void LCDShow(void);
void LCDshowMode(void);
void LCDshowConstant(void);
void LCDshowVIout(void);
void LCDshowVIref(void);
void LCDshowState(void);
void LCDshowErr(void);
void LCDshowCCV(void);
void LCDshowMonitor(void);


#endif
