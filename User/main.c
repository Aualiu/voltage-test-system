#include "stm32f10x.h"
#include "serial.h"
#include "12864.h"
#include <stdio.h>
#include "MyRTC.h"
#include "LED.h"
#include "AD.h"
#include "Timer.h"
#include "Key.h"
static uint8_t usart_tx_flage=0;
int main(void)
{
    char disp_buf[16];
    static uint16_t last_time[6] = {0}; // 记录上一次显示的时间
    static FloatData_t last_fd; // 记录上一次的浮点数据
    static int has_last_data = 0; // 是否有上一次的数据
    
    
    Init_12864();
    Key_Init();
    LED_Init();
    Serial_Init();
    MyRTC_Init();
    AD_Init();
    Timer_Init();  // 初始化定时器，每1秒触发中断发送B0_DefaultData


    while(1)
    {

        
        // 使用实验1模式：单输入G1（PA3）和5个档位按键控制输出（PA0）
        // 档位：-2, -1, 0, +1, +2 对应按键PE15, PE14, PE13, PE12, PE11
        // 其他通道固定为1.00V，保持阈值检测、LED控制和数据包发送逻辑
        experiment_mode();


       

  
        if (usart_tx_flage==1)
        {
            Serial_Send_B8_Packet(Send_B8_DefaultData, sizeof(Send_B8_DefaultData));
            Serial_Send_B0_Packet(B0_DefaultData, sizeof(B0_DefaultData));
            usart_tx_flage=0;

        }
        

        // 等待完整一包数据到达（C0 ... C0 0D 0A 或 C1 ... C1 0D 0A）
        if(Serial_IsPacketReady())
        {
            uint8_t *raw_buf = Serial_Get_Raw_Buf();
            uint16_t len = Serial_Get_Raw_Count();

            write_12864com(0x01); // 12864清屏

            // 解析 C0 数据包为两个浮点数
            Serial_ParsePacket();
            FloatData_t *fd = Serial_GetParsedFloatData();

            // 记录解析后的数据
            last_fd = *fd;
            has_last_data = 1;

            // 显示解析后的浮点数（手动格式化，避免嵌入式 printf 不支持 %.2f）
            int int1 = (int)fd->value1;
            int frac1 = (int)((fd->value1 - int1) * 100 + 0.5f); // 四舍五入
            int int2 = (int)fd->value2;
            int frac2 = (int)((fd->value2 - int2) * 100 + 0.5f);

            sprintf(led_up_data, "%d.%02d", int1, frac1);
            sprintf(led_low_data, "%d.%02d", int2, frac2);
            sprintf(disp_buf, "%d.%02d %d.%02d", int1, frac1, int2, frac2);
            Display_string(0, 1, (uint8_t*)disp_buf);


            Serial_ClearPacket();
        }
        else if(Serial_IsPacketReadyC1())
        {
            uint8_t *raw_buf = Serial_Get_Raw_BufC1();
            uint16_t len = Serial_Get_Raw_CountC1();

            write_12864com(0x01); // 12864清屏


            Serial_ParsePacketC1();
            DateTime_t *dt = Serial_GetParsedDateTimeC1();
			

            // 显示解析后的日期时间

            MyRTC_SetDateTime(dt->year, dt->month, dt->day, dt->hour, dt->minute, dt->second);

            sprintf(disp_buf, "%04d-%02d-%02d", dt->year, dt->month, dt->day);
            Display_string(0, 0, (uint8_t*)disp_buf); // 第1行第0列
            sprintf(disp_buf, "%02d:%02d:%02d", dt->hour, dt->minute, dt->second);
            Display_string(9, 1, (uint8_t*)disp_buf); // 第2行第0列

            Serial_ClearPacketC1();
        }
        else
        {
            // 如果没有串口数据，读取RTC并显示时间（仅当时间改变时更新）
            MyRTC_ReadTime();
            if (MyRTC_Time[0] != last_time[0] || MyRTC_Time[1] != last_time[1] || 
                MyRTC_Time[2] != last_time[2] || MyRTC_Time[3] != last_time[3] || 
                MyRTC_Time[4] != last_time[4] || MyRTC_Time[5] != last_time[5])
            {
                
                
                write_12864com(0x01); // 12864清屏
                // 解析实时RTC时间到16进制数组timer_data
                LED_ParseCurrentTimeToHex();
                sprintf(disp_buf, "%04d-%02d-%02d", MyRTC_Time[0], MyRTC_Time[1], MyRTC_Time[2]);
                Display_string(0, 0, (uint8_t*)disp_buf); // 第1行第0列
                sprintf(disp_buf, "%02d:%02d:%02d", MyRTC_Time[3], MyRTC_Time[4], MyRTC_Time[5]);
                Display_string(9, 1, (uint8_t*)disp_buf); // 第2行第0列
				
                
                // 如果有上一次的数据，显示在时间下面
                if (has_last_data)
                {
                    int int1 = (int)last_fd.value1;
                    int frac1 = (int)((last_fd.value1 - int1) * 100 + 0.5f);
                    int int2 = (int)last_fd.value2;
                    int frac2 = (int)((last_fd.value2 - int2) * 100 + 0.5f);
                    sprintf(disp_buf, "%d.%02d %d.%02d", int1, frac1, int2, frac2);
                    Display_string(0, 1, (uint8_t*)disp_buf); // 第3行第0列
                }
                
                // 更新上一次时间
                for (int i = 0; i < 6; i++)
                {
                    last_time[i] = MyRTC_Time[i];
                }
            }
        }

    }
}
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		usart_tx_flage=1;
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
