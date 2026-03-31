#ifndef __MYRTC_H
#define __MYRTC_H

#include "stm32f10x.h"

extern uint16_t MyRTC_Time[];

void MyRTC_Init(void);
void MyRTC_SetTime(void);
void MyRTC_ReadTime(void);
void MyRTC_SetDateTime(uint16_t Year, uint8_t Month, uint8_t Day, 
                       uint8_t Hour, uint8_t Minute, uint8_t Second);

#endif
