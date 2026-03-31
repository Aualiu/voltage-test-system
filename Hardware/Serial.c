#include "serial.h"
#include <string.h>

#define RAW_BUF_SIZE 64
static uint8_t Raw_Buf[RAW_BUF_SIZE] = {0};
static volatile uint16_t Raw_Count = 0; // 包长度（不含帧头/帧尾）
static volatile uint8_t Packet_Ready = 0; // 收到完整一包后置1，需要调用方清除


static FloatData_t ParsedFloatData = {0.0f, 0.0f};


static uint8_t Raw_Buf_C1[RAW_BUF_SIZE] = {0};
static volatile uint16_t Raw_Count_C1 = 0;
static volatile uint8_t Packet_Ready_C1 = 0;


static DateTime_t ParsedDateTimeC1 = {0};

typedef enum
{
    RX_STATE_WAIT_START = 0,
    RX_STATE_IN_PACKET,
    RX_STATE_WAIT_CR,
    RX_STATE_WAIT_LF
} RX_State_t;

static volatile RX_State_t RxState = RX_STATE_WAIT_START;
static volatile RX_State_t RxState_C1 = RX_STATE_WAIT_START;

void Serial_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    // TX -> PA9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // RX -> PA10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 波特率确认：这里设为9600
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // 最高优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);
}

// 帧解析：支持 C0 <payload> C0 0D 0A 和 C1 <payload> C1 0D 0A

void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        uint8_t RxData = USART_ReceiveData(USART1);

        // 处理 C0 包
        if(Packet_Ready == 0)
        {
            switch(RxState)
            {
                case RX_STATE_WAIT_START:
                    if(RxData == 0xC0)
                    {
                        memset(Raw_Buf, 0, sizeof(Raw_Buf));
                        Raw_Count = 0;
                        RxState = RX_STATE_IN_PACKET;
                    }
                    break;

                case RX_STATE_IN_PACKET:
                    if(RxData == 0xC0)
                    {
                        RxState = RX_STATE_WAIT_CR;
                    }
                    else if(Raw_Count < RAW_BUF_SIZE)
                    {
                        Raw_Buf[Raw_Count++] = RxData;
                    }
                    break;

                case RX_STATE_WAIT_CR:
                    if(RxData == 0x0D)
                    {
                        RxState = RX_STATE_WAIT_LF;
                    }
                    else if(RxData == 0xC0)
                    {
                        Raw_Count = 0;
                        RxState = RX_STATE_IN_PACKET;
                    }
                    else
                    {
                        RxState = RX_STATE_WAIT_START;
                    }
                    break;

                case RX_STATE_WAIT_LF:
                    if(RxData == 0x0A)
                    {
                        Packet_Ready = 1;
                    }
                    RxState = RX_STATE_WAIT_START;
                    break;

                default:
                    RxState = RX_STATE_WAIT_START;
                    break;
            }
        }

        // 处理 C1 包（如果 C0 没在处理中）
        if(Packet_Ready_C1 == 0)
        {
            switch(RxState_C1)
            {
                case RX_STATE_WAIT_START:
                    if(RxData == 0xC1)
                    {
                        memset(Raw_Buf_C1, 0, sizeof(Raw_Buf_C1));
                        Raw_Count_C1 = 0;
                        RxState_C1 = RX_STATE_IN_PACKET;
                    }
                    break;

                case RX_STATE_IN_PACKET:
                    if(RxData == 0xC1)
                    {
                        RxState_C1 = RX_STATE_WAIT_CR;
                    }
                    else if(Raw_Count_C1 < RAW_BUF_SIZE)
                    {
                        Raw_Buf_C1[Raw_Count_C1++] = RxData;
                    }
                    break;

                case RX_STATE_WAIT_CR:
                    if(RxData == 0x0D)
                    {
                        RxState_C1 = RX_STATE_WAIT_LF;
                    }
                    else if(RxData == 0xC1)
                    {
                        Raw_Count_C1 = 0;
                        RxState_C1 = RX_STATE_IN_PACKET;
                    }
                    else
                    {
                        RxState_C1 = RX_STATE_WAIT_START;
                    }
                    break;

                case RX_STATE_WAIT_LF:
                    if(RxData == 0x0A)
                    {
                        Packet_Ready_C1 = 1;
                    }
                    RxState_C1 = RX_STATE_WAIT_START;
                    break;

                default:
                    RxState_C1 = RX_STATE_WAIT_START;
                    break;
            }
        }

        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

uint16_t Serial_Get_Raw_Count(void)
{
    return Raw_Count;
}

uint8_t* Serial_Get_Raw_Buf(void)
{
    return Raw_Buf;
}

uint8_t Serial_IsPacketReady(void)
{
    return Packet_Ready;
}

void Serial_ClearPacket(void)
{
    Packet_Ready = 0;
    Raw_Count = 0;
    memset(Raw_Buf, 0, sizeof(Raw_Buf));
}

void Serial_ParsePacket(void)
{
    // 解析 C0 数据包：ASCII 格式的两个浮点数，无空格分隔，如 "1.000.96"
    // Payload: 31 2E 30 30 30 2E 39 36 -> "1.000.96"
    // 解析为：value1 = 1.00, value2 = 0.96

    if(Raw_Count >= 8) // 至少 8 字节："1.000.96"
    {
        // 转换为 ASCII 字符串
        char ascii_buf[RAW_BUF_SIZE + 1] = {0};
        for(uint16_t i = 0; i < Raw_Count && i < RAW_BUF_SIZE; i++)
        {
            ascii_buf[i] = (char)Raw_Buf[i];
        }


        if(Raw_Count >= 8)
        {

            int dot1_pos = -1;
            for(int i = 0; i < Raw_Count; i++)
            {
                if(ascii_buf[i] == '.')
                {
                    dot1_pos = i;
                    break;
                }
            }

            if(dot1_pos > 0 && dot1_pos < Raw_Count - 5) // 确保有足够字符
            {

                int int_part1 = ascii_buf[0] - '0'; // 假设一位整数
                int frac_part1 = (ascii_buf[dot1_pos + 1] - '0') * 10 + (ascii_buf[dot1_pos + 2] - '0');
                ParsedFloatData.value1 = int_part1 + frac_part1 / 100.0f;

                // value2: 从 dot1_pos+3 开始，找到下一个 '.'
                int dot2_pos = -1;
                for(int i = dot1_pos + 3; i < Raw_Count; i++)
                {
                    if(ascii_buf[i] == '.')
                    {
                        dot2_pos = i;
                        break;
                    }
                }

                if(dot2_pos > dot1_pos + 3 && dot2_pos + 2 < Raw_Count)
                {
                    int int_part2 = ascii_buf[dot1_pos + 3] - '0'; // 假设一位整数
                    int frac_part2 = (ascii_buf[dot2_pos + 1] - '0') * 10 + (ascii_buf[dot2_pos + 2] - '0');
                    ParsedFloatData.value2 = int_part2 + frac_part2 / 100.0f;
                }
                else
                {
                    ParsedFloatData.value2 = 0.0f;
                }
            }
            else
            {
                ParsedFloatData.value1 = 0.0f;
                ParsedFloatData.value2 = 0.0f;
            }
        }
        else
        {
            ParsedFloatData.value1 = 0.0f;
            ParsedFloatData.value2 = 0.0f;
        }
    }
    else
    {
        // 数据不足，设为默认值
        ParsedFloatData.value1 = 0.0f;
        ParsedFloatData.value2 = 0.0f;
    }
}

FloatData_t* Serial_GetParsedFloatData(void)
{
    return &ParsedFloatData;
}

// C1 packet functions
uint16_t Serial_Get_Raw_CountC1(void)
{
    return Raw_Count_C1;
}

uint8_t* Serial_Get_Raw_BufC1(void)
{
    return Raw_Buf_C1;
}

uint8_t Serial_IsPacketReadyC1(void)
{
    return Packet_Ready_C1;
}

void Serial_ClearPacketC1(void)
{
    Packet_Ready_C1 = 0;
    Raw_Count_C1 = 0;
    memset(Raw_Buf_C1, 0, sizeof(Raw_Buf_C1));
}

void Serial_ParsePacketC1(void)
{
    // 解析 C1 数据包：ASCII 格式的日期时间 YYYYMMDDHHMMSS
    // Payload: 32 30 32 36 30 33 31 35 31 35 32 31 33 39 -> "20260315152139"
    // 解析为：2026-03-15 15:21:39

    if(Raw_Count_C1 >= 14) // 至少 14 字节：YYYYMMDDHHMMSS
    {
        // 转换为 ASCII 字符
        char ascii_buf[15] = {0};
        for(uint16_t i = 0; i < Raw_Count_C1 && i < 14; i++)
        {
            ascii_buf[i] = (char)Raw_Buf_C1[i];
        }

        // 解析年月日时分秒
        ParsedDateTimeC1.year   = (ascii_buf[0] - '0') * 1000 +
                                  (ascii_buf[1] - '0') * 100 +
                                  (ascii_buf[2] - '0') * 10 +
                                  (ascii_buf[3] - '0');

        ParsedDateTimeC1.month  = (ascii_buf[4] - '0') * 10 +
                                  (ascii_buf[5] - '0');

        ParsedDateTimeC1.day    = (ascii_buf[6] - '0') * 10 +
                                  (ascii_buf[7] - '0');

        ParsedDateTimeC1.hour   = (ascii_buf[8] - '0') * 10 +
                                  (ascii_buf[9] - '0');

        ParsedDateTimeC1.minute = (ascii_buf[10] - '0') * 10 +
                                  (ascii_buf[11] - '0');

        ParsedDateTimeC1.second = (ascii_buf[12] - '0') * 10 +
                                  (ascii_buf[13] - '0');
    }
    else
    {
        // 数据不足，设为默认值
        ParsedDateTimeC1.year = 0;
        ParsedDateTimeC1.month = 0;
        ParsedDateTimeC1.day = 0;
        ParsedDateTimeC1.hour = 0;
        ParsedDateTimeC1.minute = 0;
        ParsedDateTimeC1.second = 0;
    }
}

DateTime_t* Serial_GetParsedDateTimeC1(void)
{
    return &ParsedDateTimeC1;
}


void Serial_Send_B8_Packet(uint8_t* data, uint16_t length)
{
    if (data == NULL || length == 0) {
        return;
    }
    

    USART_SendData(USART1, 0xB8);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    

    for (uint16_t i = 0; i < length; i++) {
        USART_SendData(USART1, data[i]);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    }
    

    USART_SendData(USART1, 0xB8);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    

    USART_SendData(USART1, 0x0D);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    
    USART_SendData(USART1, 0x0A);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}


// void Serial_Send_B8_DefaultData(void)
// {
//     uint8_t default_data[] = {0x30, 0x30, 0x30, 0x31, 0x37, 0x30, 0x30, 0x30, 0x39};
//     Serial_Send_B8_Packet(default_data, sizeof(default_data));
// }


void Serial_Send_B0_Packet(uint8_t* data, uint16_t length)
{
    if (data == NULL || length == 0) {
        return;
    }
    

    USART_SendData(USART1, 0xB0);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    

    for (uint16_t i = 0; i < length; i++) {
        USART_SendData(USART1, data[i]);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    }
    

    USART_SendData(USART1, 0xB0);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    

    USART_SendData(USART1, 0x0D);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    
    USART_SendData(USART1, 0x0A);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}


// void Serial_Send_B0_DefaultData(void)
// {
//     uint8_t default_data[104] = {
//         // Line 1: "3.84023:59:59"
//         0x33, 0x2E, 0x38, 0x34, 0x31, 0x32, 0x33, 0x3A, 0x35, 0x39, 0x3A, 0x35, 0x39,
//         // Line 2: "2.50023:59:59"  
//         0x32, 0x2E, 0x35, 0x30, 0x31, 0x32, 0x33, 0x3A, 0x35, 0x39, 0x3A, 0x35, 0x39,
//         // Line 3: "2.50023:59:59"
//         0x32, 0x2E, 0x35, 0x30, 0x31, 0x32, 0x33, 0x3A, 0x35, 0x39, 0x3A, 0x35, 0x39,
//         // Line 4: "2.50023:59:59"
//         0x32, 0x2E, 0x35, 0x30, 0x31, 0x32, 0x33, 0x3A, 0x35   , 0x39, 0x3A, 0x35, 0x39,
//         // Line 5: "2.50023:59:59"
//         0x32, 0x2E, 0x35, 0x30, 0x31, 0x32, 0x33, 0x3A, 0x35, 0x39, 0x3A, 0x35, 0x39,
//         // Line 6: "2.50023:59:59"
//         0x32, 0x2E, 0x35, 0x30, 0x31, 0x32, 0x33, 0x3A, 0x35, 0x39, 0x3A, 0x35, 0x39,
//         // Line 7: "2.50023:59:59"
//         0x32, 0x2E, 0x35, 0x30, 0x30, 0x32, 0x33, 0x3A, 0x35, 0x39, 0x3A, 0x35, 0x39,
//         // Line 8: "2.50023:59:59"
//         0x32, 0x2E, 0x35, 0x30, 0x31, 0x32, 0x33, 0x3A, 0x35, 0x39, 0x3A, 0x35, 0x39
//     };
//     Serial_Send_B0_Packet(default_data, sizeof(default_data));
// }
