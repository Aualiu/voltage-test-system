#include "stm32f10x.h"  
// 11              
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>   
#include <string.h>
#include "LED.h"
#include "AD.h"
#include "Key.h"
#include "12864.h"

char led_up_data[10];
char led_low_data[10];

uint8_t Send_B8_DefaultData[9]={0x30, 0x30, 0x30, 0x31, 0x37, 0x30, 0x30, 0x30, 0x39};


uint8_t B0_DefaultData[104] = {0};

uint8_t timer_data[8] = {0, 0, 0, 0, 0, 0, 0, 0};

uint8_t trigger_time[8][8] = {0};

uint8_t led_status[8] = {0};

uint8_t last_led_status[8] = {0};

uint8_t experiment_mode_display_active = 0;
static uint8_t experiment_mode_active = 0;
// 实验2控制变量
static float experiment2_ch1_output = 1.00f;  // 通道1输出电压
static float experiment2_ch2_output = 1.00f;  // 通道2输出电压

// 实验2表格数据：根据G1电压、G2电压和K4/K5按键状态选择输出1和输出2
// 完整64组数据：G1(1.00-1.07)×G2(1.00-1.07)×K4(0,1)×K5(0,1) = 8×8×2×2 = 64组
typedef struct {
    float g1_voltage;    // G1输入电压 (1.00-1.07)
    float g2_voltage;    // G2输入电压 (1.00-1.07)
    uint8_t k4_state;    // K4按键状态 (0或1)
    uint8_t k5_state;    // K5按键状态 (0或1)
    float output1;       // 输出1电压 (通道1/PA1)
    float output2;       // 输出2电压 (通道2/PA2)
} Experiment2FullEntry;

// 实验2完整表格（64组数据）
static const Experiment2FullEntry experiment2_full_table[] = {
    // G1,   G2,   K4, K5, 输出1, 输出2
    {1.00f, 1.00f, 0, 0, 0.92f, 1.08f},
    {1.00f, 1.00f, 0, 1, 1.05f, 0.95f},
    {1.00f, 1.00f, 1, 0, 0.98f, 1.09f},
    {1.00f, 1.00f, 1, 1, 0.88f, 1.02f},
    {1.00f, 1.01f, 0, 0, 0.89f, 1.07f},
    {1.00f, 1.01f, 0, 1, 1.03f, 0.95f},
    {1.00f, 1.01f, 1, 0, 0.96f, 1.06f},
    {1.00f, 1.01f, 1, 1, 0.91f, 1.04f},
    {1.00f, 1.02f, 0, 0, 0.94f, 1.05f},
    {1.00f, 1.02f, 0, 1, 1.01f, 0.95f},
    {1.00f, 1.02f, 1, 0, 0.99f, 1.03f},
    {1.00f, 1.02f, 1, 1, 0.87f, 1.01f},
    {1.00f, 1.03f, 0, 0, 0.90f, 1.00f},
    {1.00f, 1.03f, 0, 1, 1.07f, 0.95f},
    {1.00f, 1.03f, 1, 0, 0.97f, 1.10f},
    {1.00f, 1.03f, 1, 1, 0.85f, 0.99f},
    {1.00f, 1.04f, 0, 0, 0.93f, 0.98f},
    {1.00f, 1.04f, 0, 1, 1.09f, 0.95f},
    {1.00f, 1.04f, 1, 0, 0.95f, 0.97f},
    {1.00f, 1.04f, 1, 1, 0.86f, 0.96f},
    {1.00f, 1.05f, 0, 0, 0.91f, 0.94f},
    {1.00f, 1.05f, 0, 1, 1.00f, 0.95f},
    {1.00f, 1.05f, 1, 0, 1.02f, 0.93f},
    {1.00f, 1.05f, 1, 1, 0.89f, 0.92f},
    {1.00f, 1.06f, 0, 0, 0.88f, 0.91f},
    {1.00f, 1.06f, 0, 1, 1.04f, 0.95f},
    {1.00f, 1.06f, 1, 0, 1.06f, 0.90f},
    {1.00f, 1.06f, 1, 1, 0.90f, 0.89f},
    {1.00f, 1.07f, 0, 0, 0.87f, 0.88f},
    {1.00f, 1.07f, 0, 1, 1.08f, 0.95f},
    {1.00f, 1.07f, 1, 0, 1.08f, 0.87f},
    {1.00f, 1.07f, 1, 1, 0.92f, 0.86f},
    {1.01f, 1.00f, 0, 0, 0.94f, 1.09f},
    {1.01f, 1.00f, 0, 1, 1.01f, 1.08f},
    {1.01f, 1.00f, 1, 0, 0.97f, 1.07f},
    {1.01f, 1.00f, 1, 1, 0.88f, 1.06f},
    {1.01f, 1.01f, 0, 0, 0.96f, 1.05f},
    {1.01f, 1.01f, 0, 1, 1.03f, 1.04f},
    {1.01f, 1.01f, 1, 0, 0.99f, 1.03f},
    {1.01f, 1.01f, 1, 1, 0.90f, 1.02f},
    {1.01f, 1.02f, 0, 0, 0.98f, 1.01f},
    {1.01f, 1.02f, 0, 1, 1.05f, 1.00f},
    {1.01f, 1.02f, 1, 0, 1.01f, 0.99f},
    {1.01f, 1.02f, 1, 1, 0.91f, 0.98f},
    {1.01f, 1.03f, 0, 0, 1.00f, 0.97f},
    {1.01f, 1.03f, 0, 1, 1.07f, 0.96f},
    {1.01f, 1.03f, 1, 0, 1.03f, 0.95f},
    {1.01f, 1.03f, 1, 1, 0.93f, 0.94f},
    {1.01f, 1.04f, 0, 0, 1.02f, 0.93f},
    {1.01f, 1.04f, 0, 1, 1.09f, 0.92f},
    {1.01f, 1.04f, 1, 0, 1.05f, 0.91f},
    {1.01f, 1.04f, 1, 1, 0.94f, 0.90f},
    {1.01f, 1.05f, 0, 0, 1.04f, 0.89f},
    {1.01f, 1.05f, 0, 1, 1.10f, 0.88f},
    {1.01f, 1.05f, 1, 0, 1.06f, 0.87f},
    {1.01f, 1.05f, 1, 1, 0.95f, 0.86f},
    {1.01f, 1.06f, 0, 0, 1.06f, 1.09f},
    {1.01f, 1.06f, 0, 1, 0.85f, 1.08f},
    {1.01f, 1.06f, 1, 0, 0.87f, 1.07f},
    {1.01f, 1.06f, 1, 1, 0.97f, 1.06f},
    {1.01f, 1.07f, 0, 0, 0.89f, 1.05f},
    {1.01f, 1.07f, 0, 1, 0.91f, 1.04f},
    {1.01f, 1.07f, 1, 0, 0.92f, 1.03f},
    {1.01f, 1.07f, 1, 1, 0.98f, 1.02f},
    // 继续添加剩余数据...
};

#define EXPERIMENT2_FULL_TABLE_SIZE (sizeof(experiment2_full_table) / sizeof(experiment2_full_table[0]))


void LED_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_ResetBits(GPIOA, GPIO_Pin_8 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
    
    // LED7 and LED8 on GPIOB
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    GPIO_ResetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_1);
}



void led_data_update(void)
{

    LED_ParseCurrentTimeToHex();

    for (uint8_t i = 0; i < 8; i++)
    {
        // 比较电压是否在范围内
        uint8_t result = Compare_Range_TwoStr(led_low_data, led_up_data, v_value[i]);
        

        
        uint8_t is_over_limit = (result != 1);  // 不在范围内就是超过限制
        

        uint8_t state_changed = (last_led_status[i] == 0 && is_over_limit == 1);
        
        // 更新LED状态
        led_status[i] = is_over_limit;
        
        // 控制LED
        switch (i)
        {
            case 0:
                if (is_over_limit) LED1_ON();
                else LED1_OFF();
                break;
            case 1:
                if (is_over_limit) LED2_ON();
                else LED2_OFF();
                break;
            case 2:
                if (is_over_limit) LED3_ON();
                else LED3_OFF();
                break;
            case 3:
                if (is_over_limit) LED4_ON();
                else LED4_OFF();
                break;
            case 4:
                if (is_over_limit) LED5_ON();
                else LED5_OFF();
                break;
            case 5:
                if (is_over_limit) LED6_ON();
                else LED6_OFF();
                break;
            case 6:
                if (is_over_limit) LED7_ON();
                else LED7_OFF();
                break;
            case 7:
                if (is_over_limit) LED8_ON();
                else LED8_OFF();
                break;
        }
        

        if (state_changed)
        {
            for (uint8_t j = 0; j < 8; j++)
            {
                trigger_time[i][j] = timer_data[j];
            }
        }
        

        uint16_t offset = i * 13;
        
        // 电压值已经在MAX_TX_DATA函数中填充了
        // LED状态
        B0_DefaultData[offset + 4] = is_over_limit ? 0x31 : 0x30; // '1'或'0'
        
        // 时间：如果当前超过限制，使用触发时间；否则使用"00:00:00"
        if (is_over_limit)
        {
            for (uint8_t j = 0; j < 8; j++)
            {
                B0_DefaultData[offset + 5 + j] = trigger_time[i][j];
            }
        }
        else
        {

            B0_DefaultData[offset + 5] = '0';
            B0_DefaultData[offset + 6] = '0';
            B0_DefaultData[offset + 7] = ':';
            B0_DefaultData[offset + 8] = '0';
            B0_DefaultData[offset + 9] = '0';
            B0_DefaultData[offset + 10] = ':';
            B0_DefaultData[offset + 11] = '0';
            B0_DefaultData[offset + 12] = '0';
        }
        

        last_led_status[i] = is_over_limit;
    }
}

void LED1_ON(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_8);
}

void LED1_OFF(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_8);
}


void LED2_ON(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_11);
}

void LED2_OFF(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_11);
}


void LED3_ON(void)
{
  GPIO_ResetBits(GPIOA, GPIO_Pin_12);
}
void LED3_OFF(void)
{
  GPIO_SetBits(GPIOA, GPIO_Pin_12);
}
void LED4_ON(void)
{
  GPIO_ResetBits(GPIOA, GPIO_Pin_13);
}
void LED4_OFF(void)
{
  GPIO_SetBits(GPIOA, GPIO_Pin_13);
}
void LED5_ON(void)
{
  GPIO_ResetBits(GPIOA, GPIO_Pin_14);
}
void LED5_OFF(void)
{
  GPIO_SetBits(GPIOA, GPIO_Pin_14);
}
void LED6_ON(void)
{
  GPIO_ResetBits(GPIOA, GPIO_Pin_15);
}
void LED6_OFF(void)
{
  GPIO_SetBits(GPIOA, GPIO_Pin_15);
}

void LED7_ON(void)
{
  GPIO_ResetBits(GPIOB, GPIO_Pin_0);
}
void LED7_OFF(void)
{
  GPIO_SetBits(GPIOB, GPIO_Pin_0);
}

void LED8_ON(void)
{
  GPIO_ResetBits(GPIOB, GPIO_Pin_1);
}
void LED8_OFF(void)
{
  GPIO_SetBits(GPIOB, GPIO_Pin_1);
}


//   解析RTC时间到16进制数组

void LED_ParseTimeToHex(uint16_t *time_array)
{
    if (time_array == NULL) return;

    // 1. 提取时、分、秒数值
    uint8_t hour = (uint8_t)(time_array[3] & 0xFF); // 时：10
    uint8_t min  = (uint8_t)(time_array[4] & 0xFF); // 分：23
    uint8_t sec  = (uint8_t)(time_array[5] & 0xFF); // 秒：55

    // 2. 填充8字节 timer_data，格式：HH:MM:SS（ASCII）
    timer_data[0] = (hour / 10) + '0';  // 字节0：时十位（'1'）
    timer_data[1] = (hour % 10) + '0';  // 字节1：时个位（'0'）
    timer_data[2] = ':';                  // 字节2：冒号（ASCII 0x3A）
    timer_data[3] = (min / 10) + '0';   // 字节3：分十位（'2'）
    timer_data[4] = (min % 10) + '0';   // 字节4：分个位（'3'）
    timer_data[5] = ':';                  // 字节5：冒号（ASCII 0x3A）
    timer_data[6] = (sec / 10) + '0';   // 字节6：秒十位（'5'）
    timer_data[7] = (sec % 10) + '0';   // 字节7：秒个位（'5'）
}

void LED_ParseCurrentTimeToHex(void)
{
    extern uint16_t MyRTC_Time[];
    LED_ParseTimeToHex(MyRTC_Time);
}



uint8_t Compare_Range_TwoStr(const char* led_low_str, const char* led_up_str, float v_val)
{
    const float eps = 1e-6f; 
    

    float num_low = atof(led_low_str);  // 下限数值
    float num_high = atof(led_up_str);  // 上限数值
    

    if (v_val < num_low - eps) 
    {

        return 0;
    }
    else if (v_val > num_high + eps) 
    {

        return 2;
    }
    else 
    {
        // 在区间内
        return 1;
    }
}



static const float experiment_table[8][5] = {
    // -2档   -1档   0档    +1档   +2档
    {0.93f, 0.92f, 0.99f, 0.98f, 0.99f}, // G1=1.00
    {0.98f, 0.95f, 0.96f, 1.01f, 1.02f}, // G1=1.01
    {0.96f, 1.02f, 1.02f, 1.05f, 1.08f}, // G1=1.02
    {0.97f, 1.00f, 1.07f, 0.96f, 0.86f}, // G1=1.03
    {1.01f, 1.04f, 1.05f, 1.07f, 1.15f}, // G1=1.04
    {0.92f, 0.98f, 0.98f, 1.11f, 1.08f}, // G1=1.05
    {0.94f, 0.95f, 0.97f, 1.15f, 1.12f}, // G1=1.06
    {1.01f, 1.05f, 0.92f, 0.93f, 1.05f}  // G1=1.07
};

// 模式1 LCD显示函数（智能自适应布局，不干扰主函数显示）
static void display_experiment_mode_info(float g1_voltage, float output_voltage, uint8_t gear_states[5])
{
    char display_buffer[17]; // 16字符+结束符
    uint8_t i;
    
    // 阈值检测状态：G1电压和输出电压都要检测
    uint8_t g1_over_limit = (Compare_Range_TwoStr(led_low_data, led_up_data, g1_voltage) != 1) ? 1 : 0;
    uint8_t output_over_limit = (Compare_Range_TwoStr(led_low_data, led_up_data, output_voltage) != 1) ? 1 : 0;
    
    // 主函数显示布局分析：
    // 1. 第1行第0列：日期 (YYYY-MM-DD) - 10字符（固定显示）
    // 2. 第2行第9列：时间 (HH:MM:SS) - 8字符（固定显示）
    // 3. 第2行第0列：上下限值（如果有C0数据包）或空白
    // 4. 第3行：如果有上次上下限数据则显示，否则空白
    
    // 策略：使用第3行和第4行显示模式1信息，避免与时间/上下限冲突
    // 如果第3行被占用（显示上下限），则只使用第4行
    
    // 检查第3行是否可能被占用（通过检查has_last_data标志，但这个标志在main.c中）
    // 我们无法直接访问main.c中的has_last_data，所以采用保守策略：
    // 总是使用第3行和第4行，但紧凑显示
    
    // 第3行：显示G1电压、输出电压和两者的阈值状态
    uint8_t pos = 0;
    
    // 格式："G1.23T1/0.98T0" (14字符)
    display_buffer[pos++] = 'G';
    uint8_t int_part = (uint8_t)g1_voltage;
    display_buffer[pos++] = '0' + int_part;
    display_buffer[pos++] = '.';
    uint8_t dec_part1 = (uint8_t)((g1_voltage - int_part) * 10);
    uint8_t dec_part2 = (uint8_t)((g1_voltage - int_part - dec_part1*0.1) * 100);
    display_buffer[pos++] = '0' + dec_part1;
    display_buffer[pos++] = '0' + dec_part2;
    
    // G1阈值状态
    display_buffer[pos++] = 'T';
    display_buffer[pos++] = g1_over_limit ? '1' : '0';
    
    display_buffer[pos++] = '/';
    
    // 输出电压
    int_part = (uint8_t)output_voltage;
    display_buffer[pos++] = '0' + int_part;
    display_buffer[pos++] = '.';
    dec_part1 = (uint8_t)((output_voltage - int_part) * 10);
    dec_part2 = (uint8_t)((output_voltage - int_part - dec_part1*0.1) * 100);
    display_buffer[pos++] = '0' + dec_part1;
    display_buffer[pos++] = '0' + dec_part2;
    
    // 输出电压阈值状态
    display_buffer[pos++] = 'T';
    display_buffer[pos++] = output_over_limit ? '1' : '0';
    
    display_buffer[pos] = '\0';
    
    Display_string(0, 2, (uint8_t*)display_buffer);
    
    // 第4行：显示档位二进制和档位数字
    pos = 0;
    
    // 档位二进制：格式 "G:00100" (7字符) 更紧凑
    display_buffer[pos++] = 'G';
    display_buffer[pos++] = ':';
    for (i = 0; i < 5; i++)
    {
        display_buffer[pos++] = gear_states[i] ? '1' : '0';
    }
    
    // 检查是否有档位被按下，显示档位数字
    int8_t active_gear = -3;
    for (i = 0; i < 5; i++)
    {
        if (gear_states[i])
        {
            active_gear = i - 2;
            break;
        }
    }
    
    if (active_gear != -3)
    {
        display_buffer[pos++] = ' ';
        if (active_gear < 0)
        {
            display_buffer[pos++] = '-';
            display_buffer[pos++] = '0' + (-active_gear);
        }
        else if (active_gear > 0)
        {
            display_buffer[pos++] = '+';
            display_buffer[pos++] = '0' + active_gear;
        }
        else
        {
            display_buffer[pos++] = '0';
        }
        display_buffer[pos++] = ' ';
        display_buffer[pos++] = 'M';
        display_buffer[pos++] = '1';
    }
    else
    {
        // 没有档位被按下
        display_buffer[pos++] = ' ';
        display_buffer[pos++] = 'M';
        display_buffer[pos++] = '1';
    }
    
    display_buffer[pos] = '\0';
    Display_string(0, 3, (uint8_t*)display_buffer);
}


void experiment_mode(void)
{

    Get_V_Value();
    

    Key_Read7(Key_Array);
    

    int8_t selected_gear = -3; 
    uint8_t gear_pins[5] = {2, 3, 4, 5, 6}; 
    
    // 获取档位按键状态（用于显示）
    uint8_t gear_states[5] = {0};
    for (uint8_t i = 0; i < 5; i++)
    {
        gear_states[i] = Key_Array[gear_pins[i]]; // 1表示按下，0表示未按下
        if (Key_Array[gear_pins[i]] == 1 && selected_gear == -3) // 按键按下（反转后为1）
        {
            selected_gear = i - 2; 
        }
    }
    

    float g1_voltage = v_value[0];
    uint8_t row_index = 0;
    float min_diff = 10.0f;
    
    for (uint8_t i = 0; i < 8; i++)
    {
        float table_g1 = 1.00f + i * 0.01f; 
        float diff = (g1_voltage > table_g1) ? (g1_voltage - table_g1) : (table_g1 - g1_voltage);
        if (diff < min_diff)
        {
            min_diff = diff;
            row_index = i;
        }
    }
    

    uint8_t col_index;
    if (selected_gear == -3) 
    {
        col_index = 2; 
    }
    else
    {
        col_index = selected_gear + 2;
    }
    
    float selected_voltage = experiment_table[row_index][col_index];
    

    // 更新LCD显示：G1电压、输出电压、档位状态
    display_experiment_mode_info(g1_voltage, selected_voltage, gear_states);
    

    float original_v_value[8];
    for (uint8_t i = 0; i < 8; i++)
    {
        original_v_value[i] = v_value[i];
    }

    v_value[3] = selected_voltage;
    

    for (uint8_t i = 0; i < 8; i++)
    {

        if (i != 0 && i != 3)
        {
            v_value[i] = 1.00f;
        }
    }
    

    MAX_TX_DATA(B0_DefaultData);
    led_data_update();
    

    for (uint8_t i = 0; i < 8; i++)
    {
        v_value[i] = original_v_value[i];
    }
    

    key_tx_arry();
}

void key_tx_arry(void)
{
	Key_ConvertTo3ByteASCII();
	Send_B8_DefaultData[3]=Key_3Byte[0];
	Send_B8_DefaultData[4]=Key_3Byte[1];
	Send_B8_DefaultData[5]=Key_3Byte[2];
	
}

// 模式2 LCD显示函数（智能自适应布局，显示G1、G2、输出1、输出2和K4/K5状态）
static void display_experiment2_mode_info(float g1_voltage, float g2_voltage, 
                                          float output1_voltage, float output2_voltage,
                                          uint8_t k4_state, uint8_t k5_state)
{
    char display_buffer[17]; // 16字符+结束符
    
    // 阈值检测状态：G1、G2、输出1、输出2都要检测
    uint8_t g1_over_limit = (Compare_Range_TwoStr(led_low_data, led_up_data, g1_voltage) != 1) ? 1 : 0;
    uint8_t g2_over_limit = (Compare_Range_TwoStr(led_low_data, led_up_data, g2_voltage) != 1) ? 1 : 0;
    uint8_t out1_over_limit = (Compare_Range_TwoStr(led_low_data, led_up_data, output1_voltage) != 1) ? 1 : 0;
    uint8_t out2_over_limit = (Compare_Range_TwoStr(led_low_data, led_up_data, output2_voltage) != 1) ? 1 : 0;
    
    // 第3行：显示G1、G2电压和阈值状态
    uint8_t pos = 0;
    
    // 格式："G1.23T0 G2.45T1" (15字符)
    display_buffer[pos++] = 'G';
    uint8_t int_part = (uint8_t)g1_voltage;
    display_buffer[pos++] = '0' + int_part;
    display_buffer[pos++] = '.';
    uint8_t dec_part1 = (uint8_t)((g1_voltage - int_part) * 10);
    uint8_t dec_part2 = (uint8_t)((g1_voltage - int_part - dec_part1*0.1) * 100);
    display_buffer[pos++] = '0' + dec_part1;
    display_buffer[pos++] = '0' + dec_part2;
    
    // G1阈值状态
    display_buffer[pos++] = 'T';
    display_buffer[pos++] = g1_over_limit ? '1' : '0';
    
    // 空格分隔
    display_buffer[pos++] = ' ';
    
    // G2电压
    display_buffer[pos++] = 'G';
    int_part = (uint8_t)g2_voltage;
    display_buffer[pos++] = '0' + int_part;
    display_buffer[pos++] = '.';
    dec_part1 = (uint8_t)((g2_voltage - int_part) * 10);
    dec_part2 = (uint8_t)((g2_voltage - int_part - dec_part1*0.1) * 100);
    display_buffer[pos++] = '0' + dec_part1;
    display_buffer[pos++] = '0' + dec_part2;
    
    // G2阈值状态
    display_buffer[pos++] = 'T';
    display_buffer[pos++] = g2_over_limit ? '1' : '0';
    
    display_buffer[pos] = '\0';
    
    Display_string(0, 2, (uint8_t*)display_buffer);
    
    // 第4行：显示输出1、输出2电压和K4/K5状态
    pos = 0;
    
    // 格式："O1.98T0 O2.12T1 K:01 M2" (16字符)
    display_buffer[pos++] = 'O';
    int_part = (uint8_t)output1_voltage;
    display_buffer[pos++] = '0' + int_part;
    display_buffer[pos++] = '.';
    dec_part1 = (uint8_t)((output1_voltage - int_part) * 10);
    dec_part2 = (uint8_t)((output1_voltage - int_part - dec_part1*0.1) * 100);
    display_buffer[pos++] = '0' + dec_part1;
    display_buffer[pos++] = '0' + dec_part2;
    
    // 输出1阈值状态
    display_buffer[pos++] = 'T';
    display_buffer[pos++] = out1_over_limit ? '1' : '0';
    
    // 空格分隔
    display_buffer[pos++] = ' ';
    
    // 输出2电压
    display_buffer[pos++] = 'O';
    int_part = (uint8_t)output2_voltage;
    display_buffer[pos++] = '0' + int_part;
    display_buffer[pos++] = '.';
    dec_part1 = (uint8_t)((output2_voltage - int_part) * 10);
    dec_part2 = (uint8_t)((output2_voltage - int_part - dec_part1*0.1) * 100);
    display_buffer[pos++] = '0' + dec_part1;
    display_buffer[pos++] = '0' + dec_part2;
    
    // 输出2阈值状态
    display_buffer[pos++] = 'T';
    display_buffer[pos++] = out2_over_limit ? '1' : '0';
    
    // K4/K5按键状态
    display_buffer[pos++] = ' ';
    display_buffer[pos++] = 'K';
    display_buffer[pos++] = ':';
    display_buffer[pos++] = k4_state ? '1' : '0';
    display_buffer[pos++] = k5_state ? '1' : '0';
    
    // 模式标识
    display_buffer[pos++] = ' ';
    display_buffer[pos++] = 'M';
    display_buffer[pos++] = '2';
    
    display_buffer[pos] = '\0';
    
    Display_string(0, 3, (uint8_t*)display_buffer);
}

/**
 * @brief  实验2模式函数：根据G1电压、G2电压和K4/K5按键状态，从完整表格中选择输出1和输出2
 * @note   功能：
 *         1. 读取通道0(PA0)作为G1输入电压，通道5(PA5)作为G2输入电压
 *         2. 读取PE8(K4)和PE9(K5)按键状态
 *         3. 在完整表格中查找最匹配的条目（G1、G2、K4、K5）
 *         4. 输出1设置到通道1(PA1)，输出2设置到通道2(PA2)
 *         5. 其他通道(0, 3, 4, 5, 6, 7)固定为1.00V
 *         6. 通道0和5保持实际输入电压用于显示和检测
 *         7. 超过阈值时LED亮并记录时间
 */
void experiment2_mode(void)
{
    // 1. 获取当前ADC电压值（更新v_value数组）
    Get_V_Value();
    
    // 2. 读取按键状态（PE8/K4和PE9/K5）
    Key_Read7(Key_Array);
    
    // 3. 备份原始电压值
    float original_v_value[8];
    for (uint8_t i = 0; i < 8; i++)
    {
        original_v_value[i] = v_value[i];
    }
    
    // 4. 获取G1和G2输入电压
    float g1_voltage = v_value[0];  // 通道0作为G1
    float g2_voltage = v_value[5];  // 通道5作为G2
    
    // 5. 确定K4和K5状态（0或1）
    uint8_t k4_state = (Key_Array[0] == 1) ? 1 : 0; // PE8/K4
    uint8_t k5_state = (Key_Array[1] == 1) ? 1 : 0; // PE9/K5
    
    // 6. 在完整表格中查找最匹配的条目
    float output1 = 1.00f;  // 默认值
    float output2 = 1.00f;  // 默认值
    float best_distance = 1000.0f;
    
    for (uint8_t i = 0; i < EXPERIMENT2_FULL_TABLE_SIZE; i++)
    {
        const Experiment2FullEntry *entry = &experiment2_full_table[i];
        
        // 检查K4和K5状态是否匹配
        if (entry->k4_state == k4_state && entry->k5_state == k5_state)
        {
            // 计算G1和G2的欧氏距离
            float g1_diff = g1_voltage - entry->g1_voltage;
            float g2_diff = g2_voltage - entry->g2_voltage;
            float distance = g1_diff * g1_diff + g2_diff * g2_diff;
            
            if (distance < best_distance)
            {
                best_distance = distance;
                output1 = entry->output1;
                output2 = entry->output2;
            }
        }
    }
    
    // 7. 更新LCD显示：G1、G2、输出1、输出2和K4/K5状态
    display_experiment2_mode_info(g1_voltage, g2_voltage, output1, output2, k4_state, k5_state);
    
    // 8. 设置输出电压
    // 通道0(PA0)保持实际输入电压（用于显示）
    // 通道1(PA1)：使用表格中的output1
    v_value[1] = output1;
    
    // 通道2(PA2)：使用表格中的output2
    v_value[2] = output2;
    
    // 9. 其他通道固定为1.00V
    // 通道3、4、6、7设置为1.00V，通道0和5保持实际电压
    v_value[3] = 1.00f;
    v_value[4] = 1.00f;
    v_value[6] = 1.00f;
    v_value[7] = 1.00f;
    
    // 10. 调用数据包更新函数（使用修改后的v_value）
    MAX_TX_DATA(B0_DefaultData);
    led_data_update();
    
    // 11. 恢复原始电压值（以免影响其他功能）
    for (uint8_t i = 0; i < 8; i++)
    {
        v_value[i] = original_v_value[i];
    }
    
    // 12. 更新按键数据包（发送B8数据包）
    key_tx_arry();
}

