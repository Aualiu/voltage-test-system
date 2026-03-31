#ifndef __LED_H
#define __LED_H
#include <stdint.h>
extern char led_up_data[10];
extern char led_low_data[10];
extern uint8_t B0_DefaultData[104];
extern uint8_t Send_B8_DefaultData[9];
extern uint8_t timer_data[8];
extern uint8_t trigger_time[8][8];
extern uint8_t led_status[8];
extern uint8_t experiment_mode_display_active;
void LED_Init(void);
void LED1_ON(void);
void LED1_OFF(void);

void LED2_ON(void);
void LED2_OFF(void);

void LED3_ON(void);
void LED3_OFF(void);
void LED4_ON(void);
void LED4_OFF(void);
void LED5_ON(void);
void LED5_OFF(void);
void LED6_ON(void);
void LED6_OFF(void);

void LED7_ON(void);
void LED7_OFF(void);
void LED8_ON(void);
void LED8_OFF(void);

void LED_ParseTimeToHex(uint16_t *time_array);
void LED_ParseCurrentTimeToHex(void);
uint8_t Compare_Range_TwoStr(const char* led_low_str, const char* led_up_str, float v_val);
void led_data_update(void);
void key_tx_arry(void);
void experiment_mode(void);
void experiment2_mode(void);
#endif
