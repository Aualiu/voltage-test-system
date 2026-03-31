#include "stm32f10x.h"
#include "Delay.h"

uint8_t Key_Array[8] = {0};      // 8个按键 0/1
uint8_t Key_ASCII[8] = {0};      // ASCII码（保留）
uint8_t Key_Hex = 0;             // 8位组合值
uint8_t Key_3Byte[3] = {0};      // 【新增】最终3字节结果：0x32,0x35,0x35

// 8个按键：PE8、PE9、PE10、PE11、PE12、PE13、PE14、PE15
void Key_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8  | GPIO_Pin_9  | GPIO_Pin_10  |
                                  GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 |
                                  GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
}


void Key_Read7(uint8_t buf[])
{
	static uint16_t last_stable = 0;
	uint16_t now = GPIOE->IDR;

	if (now != last_stable)
	{
		Delay_ms(20);
		now = GPIOE->IDR;
		last_stable = now;
	}


	

	buf[0] = ((now >> 9)  & 1) ^ 1;  // 物理PE9 → 逻辑PE8 (K4)
	

	buf[1] = ((now >> 10) & 1) ^ 1;  // 物理PE10 → 逻辑PE9 (K5)
	

	buf[2] = ((now >> 15) & 1) ^ 1;  // 物理PE15 → 逻辑PE10

	buf[3] = ((now >> 14) & 1) ^ 1;  // 物理PE14 → 逻辑PE11

	buf[4] = ((now >> 13) & 1) ^ 1;  // 物理PE13 → 逻辑PE12
	
	// 读取物理PE12（代替原先PE13）作为逻辑PE13 (档位+1)
	buf[5] = ((now >> 12) & 1) ^ 1;  // 物理PE12 → 逻辑PE13
	

	buf[6] = ((now >> 11) & 1) ^ 1;  // 物理PE11 → 逻辑PE14

	buf[7] = ((now >> 8)  & 1) ^ 1;  // 物理PE8 → 逻辑PE15
}


void Key_ConvertToASCII(void)
{
    uint8_t i;
    Key_Read7(Key_Array);

    for(i = 0; i < 8; i++)
    {
        if(Key_Array[i] == 1)  // 按下时为1（反转后）
            Key_ASCII[i] = 0x31; // '1'
        else
            Key_ASCII[i] = 0x30; // '0'
    }
}

// 8位组合成1字节
void Key_ConvertToHex(void)
{
    Key_Read7(Key_Array);
    Key_Hex = 0;
    Key_Hex |= Key_Array[0] << 0;
    Key_Hex |= Key_Array[1] << 1;
    Key_Hex |= Key_Array[2] << 2;
    Key_Hex |= Key_Array[3] << 3;
    Key_Hex |= Key_Array[4] << 4;
    Key_Hex |= Key_Array[5] << 5;
    Key_Hex |= Key_Array[6] << 6;
    Key_Hex |= Key_Array[7] << 7;
}

// ======================================================================
// 功能：
// 1. 调用 Key_Read7 存入8位0/1
// 2. 组合 → 转十进制 → 拆成3个ASCII字节（如 255 → 0x32,0x35,0x35）
// 3. 结果存入 Key_3Byte[0]、Key_3Byte[1]、Key_3Byte[2]
// ======================================================================
void Key_ConvertTo3ByteASCII(void)
{

	uint16_t temp = 0;	// 临时存储8位组合值 0~255

	// 1. 读取按键状态到 Key_Array
	Key_Read7(Key_Array);

	temp = 0;
	temp |= Key_Array[0] << 0;
	temp |= Key_Array[1] << 1;
	temp |= Key_Array[2] << 2;
	temp |= Key_Array[3] << 3;
	temp |= Key_Array[4] << 4;
	temp |= Key_Array[5] << 5;
	temp |= Key_Array[6] << 6;
	temp |= Key_Array[7] << 7;

	// 3. 分解成 百位、十位、个位 → 转ASCII
	Key_3Byte[0] = (temp / 100) + '0';    // 百位
	Key_3Byte[1] = (temp / 10) % 10 + '0';// 十位
	Key_3Byte[2] = (temp % 10) + '0';     // 个位
}
