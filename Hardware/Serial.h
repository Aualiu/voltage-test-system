#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"

void Serial_Init(void);
uint16_t Serial_Get_Raw_Count(void);
uint8_t* Serial_Get_Raw_Buf(void);

uint8_t Serial_IsPacketReady(void);
void Serial_ClearPacket(void);
void Serial_ParsePacket(void); 


typedef struct {
    float value1;
    float value2;
} FloatData_t;

FloatData_t* Serial_GetParsedFloatData(void);


uint16_t Serial_Get_Raw_CountC1(void);
uint8_t* Serial_Get_Raw_BufC1(void);
uint8_t Serial_IsPacketReadyC1(void);
void Serial_ClearPacketC1(void);
void Serial_ParsePacketC1(void); 


typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} DateTime_t;

DateTime_t* Serial_GetParsedDateTimeC1(void);


void Serial_Send_B8_Packet(uint8_t* data, uint16_t length);
// void Serial_Send_B8_DefaultData(void);


void Serial_Send_B0_Packet(uint8_t* data, uint16_t length);
// void Serial_Send_B0_DefaultData(void);

#endif
