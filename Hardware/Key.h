#ifndef __KEY_H
#define __KEY_H

void Key_Init(void);
void Key_Read7(uint8_t buf[]);
void Key_ConvertToASCII(void);
void Key_ConvertToHex(void);
void Key_ConvertTo3ByteASCII(void);


extern uint8_t Key_Hex;
extern uint8_t Key_3Byte[3];
extern uint8_t Key_ASCII[8];
extern uint8_t Key_Array[8];
#endif
