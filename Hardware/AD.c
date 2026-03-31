#include "stm32f10x.h"                
#include <stddef.h>  
#include <stdint.h>  
#include <stdio.h>
#include <string.h>
#include "AD.h"
#include "LED.h"
uint16_t AD_Value[8];
float v_value[8]={0,0,0,0,0,0,0,0};
uint8_t Voltage_Storage[32]={0x30, 0x2E, 0x39, 0x38, 0x30, 0x2E, 0x39, 0x38, 0x30, 0x2E, 0x39, 0x38, 0x30, 0x2E, 0x39, 0x38,0x30, 0x2E, 0x39, 0x38, 0x30, 0x2E, 0x39, 0x38, 0x30, 0x2E, 0x39, 0x38,0x30, 0x2E, 0x39, 0x38}; 

void AD_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);	//开启ADC1的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//开启GPIOA的时钟
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);		//DMA是AHB总线的设备，所以要用AHB开启时钟的函数

	
	/*设置ADC时钟*/
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);						//选择时钟6分频，ADCCLK = 72MHz / 6 = 12MHz
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;	//模拟输入，此时GPIO口无效，防止GPIO口的输入输出对模拟电压造成干扰
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA0引脚初始化为模拟输入

	/*规则组通道配置*/
	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 3, ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 4, ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 5, ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 6, ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 7, ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_7, 8, ADC_SampleTime_55Cycles5);

	/*ADC初始化*/
	ADC_InitTypeDef ADC_InitStructure;						//定义结构体变量
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;		//模式，选择独立模式，即单独使用ADC1
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	//数据对齐，选择右对齐
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//外部触发，使用软件触发，不需要外部触发
	//ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;		//单次转换，失能，每转换一次规则组序列后停止
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;	//连续转换
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;			//扫描模式
	ADC_InitStructure.ADC_NbrOfChannel = 8;					//通道数，为8个通道
	ADC_Init(ADC1, &ADC_InitStructure);						//将结构体变量交给ADC_Init，配置ADC1
	
	
	/*DMA初始化*/
	DMA_InitTypeDef DMA_InitStructure;										//定义结构体变量
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;			//外设基地址,源头来自ADC1的dr寄存器(0x4001244C)
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;	//外设数据宽度，选择字节(HalfWord、Word)，DR寄存器里底16位是ADC1数据存储的地方
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;			//外设地址不自增，因为数据来源地址始终是DR寄存器里底16位
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)AD_Value;					//存储器基地址，目的地
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;			//存储器数据宽度，选择字节
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;					//存储器地址自增，选择使能
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;						//数据传输方向，选择由外设到存储器,外设作为源头(DMA_DIR_PeripheralSDSC:外设作为目的地)
	DMA_InitStructure.DMA_BufferSize = 8;								//转运的数据大小（转运次数，也就是传输计数器）与ADC通道数一致
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;							//模式，选择循环模式(是否使用自动重装：DMA_Mode_Circular)
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;								//外设到存储器，硬件触发，选择失能
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;					//优先级，选择中等(VeryHigh,High,Medium,Low)
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);							//哪个DMA，哪个通道(硬件触发的通道要看PPT104)
	
	/*DMA使能*/
	DMA_Cmd(DMA1_Channel1, ENABLE);
	ADC_DMACmd(ADC1, ENABLE);	
	/*ADC使能*/
	ADC_Cmd(ADC1, ENABLE);
	
	/*ADC校准，软件置为1，那硬件就会开始复位校准，校准完成后，由硬件自动清0*/
	ADC_ResetCalibration(ADC1);								//固定流程，内部有电路会自动执行校准
	/*set通常表示 “设置” 或 “使能” 状态。
	在逻辑上，它的值一般被定义为 1，用来表示某个功能或标志被激活、启用或处于高电平状态。*/
	while (ADC_GetResetCalibrationStatus(ADC1) == SET);	//校准完成后为0(SET相当于1，RESET反之)，获取的就是CR2寄存器里的RSTTCAL标志位
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1) == SET);
	
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);	//连续转换时触发一次即可

}


//调用该函数后，v_value数组中成功存入电压值(浮点类型)
// 注意：由于硬件接线错误，在软件层面进行通道交换：
// 1. PA0 (通道0) 和 PA3 (通道3) 交换
// 2. PA5 (通道5) 和 PA6 (通道6) 交换
void ad_change_v(uint16_t ad_value[])
{
	uint8_t i=0;
	uint16_t swapped_ad_value[8];
	
	// 执行软件通道交换
	// 原始映射：AD_Value[0]=PA0, [1]=PA1, [2]=PA2, [3]=PA3, [4]=PA4, [5]=PA5, [6]=PA6, [7]=PA7
	// 交换后映射：
	swapped_ad_value[0] = ad_value[3];  // PA0 ← 实际PA3的ADC值
	swapped_ad_value[1] = ad_value[1];  // PA1 不变
	swapped_ad_value[2] = ad_value[2];  // PA2 不变
	swapped_ad_value[3] = ad_value[0];  // PA3 ← 实际PA0的ADC值
	swapped_ad_value[4] = ad_value[4];  // PA4 不变
	swapped_ad_value[5] = ad_value[6];  // PA5 ← 实际PA6的ADC值
	swapped_ad_value[6] = ad_value[5];  // PA6 ← 实际PA5的ADC值
	swapped_ad_value[7] = ad_value[7];  // PA7 不变
	
	for(i=0;i<8;i++)
	{
		float temp =0.95+(float)swapped_ad_value[i]/ 4095 * 0.3;
		if (temp - v_value[i] > 0.003 || v_value[i] - temp > 0.003)
		{
				v_value[i]=temp;
		}
//		v_value[i]=(float)ad_value[i]/ 4095 * 3.3;
	}
}


void Get_V_Value(void)
{
	ad_change_v(AD_Value);
}


 

/**
 * @brief  电压值解析函数（和LED_ParseTimeToHex风格完全对齐）
 * @param  v_array 输入的8个float电压值数组（如{3.33,3.21,...}）
 * @note   每个float占4字节，按内存原始字节序写入Voltage_Storage：
 *         v_array[0] → 0~3字节 | v_array[1] →4~7字节 | ... | v_array[7]→28~31字节
 */
void LED_ParseVoltageToHex(float *v_array)
{
    // 1. 空指针校验
    if (v_array == NULL) return;
    
    // 临时缓冲区：足够容纳"9.99"（4字符+结束符）
    char ascii_buf[8] = {0};
    
    // 2. 遍历8个电压值
    for (uint8_t i = 0; i < 8; i++)
    {
        // 步骤1：电压范围钳位（0.00~9.99，适配4字节"X.XX"格式）
        float volt_clamped = v_array[i];
       
        
        // 步骤2：【核心修改】严格截断两位小数（无四舍五入）
        // 逻辑：先乘100转整数（截断小数），再除以100.0f还原为两位小数
        uint16_t volt_int_part = (uint16_t)(volt_clamped * 100.0f); // 3.149→314，5.6789→567
        float volt_truncated = (float)volt_int_part / 100.0f;         // 314→3.14，567→5.67
        
        // 步骤3：强制格式化为"X.XX"字符串（确保4个字符）
        sprintf(ascii_buf, "%.2f", volt_truncated);
        
        // 步骤4：计算存储起始地址（和原逻辑一致：i×4）
        uint16_t start_addr = i * 4;
        
        // 步骤5：ASCII字符转16进制（严格取4个有效字符）
        Voltage_Storage[start_addr]     = (uint8_t)ascii_buf[0]; // 整数位（如'3'→0x33）
        Voltage_Storage[start_addr + 1] = (uint8_t)ascii_buf[1]; // 小数点（'.'→0x2E）
        Voltage_Storage[start_addr + 2] = (uint8_t)ascii_buf[2]; // 小数第一位
        Voltage_Storage[start_addr + 3] = (uint8_t)ascii_buf[3]; // 小数第二位
        
        // 步骤6：清空缓冲区，避免脏数据
        memset(ascii_buf, 0, sizeof(ascii_buf));
    }
}


/**
 * @brief  填充104字节数据包中的电压数据部分
 * @param  data: 104字节数据包数组
 * @note   将8个电压值（每个4字节ASCII）填充到数据包的对应位置
 *         每组13字节：0-3:电压，4:LED状态，5-12:时间
 *         电压数据从Voltage_Storage[32]中获取
 */
void MAX_TX_DATA(uint8_t data[104])
{
	LED_ParseVoltageToHex(v_value);
	
	// 遍历8个通道
	for (uint8_t i = 0; i < 8; i++)
	{
		uint16_t offset = i * 13;  // 每组13字节
		uint16_t volt_offset = i * 4;  // 每个电压4字节在Voltage_Storage中的偏移
		
		// 填充电压数据（4字节ASCII）
		data[offset + 0] = Voltage_Storage[volt_offset + 0];
		data[offset + 1] = Voltage_Storage[volt_offset + 1];
		data[offset + 2] = Voltage_Storage[volt_offset + 2];
		data[offset + 3] = Voltage_Storage[volt_offset + 3];
		
		// LED状态和时间部分由led_data_update()函数填充
		// 这里不做处理，避免覆盖
	}
}
// 调用示例（和时间解析的调用方式完全对齐）
// void Test_VoltageParse(void)
// {
//     // 1. 定义电压数组（你的v_value）
//     float v_value[8] = {3.33f, 3.21f, 5.67f, 9.80f, 1.23f, 4.56f, 7.89f, 0.99f};
    
//     // 2. 调用解析函数，写入32字节全局数组（和LED_ParseTimeToHex调用一致）
//     LED_ParseVoltageToHex(v_value);
    

// }

