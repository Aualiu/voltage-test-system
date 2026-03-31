#ifndef __AD_H
#define __AD_H

extern uint16_t AD_Value[8];	//作为一个外部可调用的数组
extern float v_value[8];
//extern uint32_t yiliu[32];
void AD_Init(void);
void AD_GetValue(void);
void ad_change_v(uint16_t ad_value[]);
void Get_V_Value(void);
extern uint8_t Voltage_Storage[32]; 
void MAX_TX_DATA(uint8_t data[104]);
#endif
