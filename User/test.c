#include "test.h"
#include <stdarg.h>
#include <stdio.h>
#include "usart.h"
#include "wifi_config.h"
#include "lcd.h"

uint16_t USART2_RX_STA = 0;
uint8_t USART2_TX_BUF[USART2_MAX_SEND_LEN];
uint8_t USART2_RX_BUF[USART2_MAX_RECV_LEN];

uint16_t USART1_RX_STA = 0;
uint8_t USART1_RX_BUF[USART1_MAX_RECV_LEN];

uint8_t STATUS;
char ANSWER = ' ';

//串口打印函数
void usart1_printf(char *fmt, ...)
{
	int i;
	char send_message[50];
	va_list ap;
	va_start(ap, fmt);
	i = vsprintf(send_message, fmt, ap);
	va_end(ap);
	if (i > 50)
	{
		return;
	}
	HAL_UART_Transmit(&huart1, (uint8_t*) send_message, i, 1000);
}

//串口打印函数
void usart2_printf(char *fmt, ...)
{
	int i;
	char send_message[50];
	va_list ap;
	va_start(ap, fmt);
	i = vsprintf(send_message, fmt, ap);
	va_end(ap);
	if (i > 50)
	{
		return;
	}
	HAL_UART_Transmit(&huart2, (uint8_t*) send_message, i, 1000);
}

void Test(void)
{
	__HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);                        //启动空闲中断
	__HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
	HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF, USART2_MAX_RECV_LEN); //开启DMA接收
	HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF, USART1_MAX_RECV_LEN);
	usart1_printf("hello\r\n");
	uint8_t mode = 0;  //两个wifi模块配置的模式为0和1，连接后TCP客户端为透传，TCP服务器是根据端口进行数据发送
	wifi_init(mode);
	STATUS = 0;
	while (1)
	{
		if (mode)
		{
			if (USART1_RX_STA == 1)
			{
				USART1_RX_STA = 0;
				usart2_printf("%s", USART1_RX_BUF);
			}
			memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
			HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF,
			USART1_MAX_RECV_LEN);
		}
		else
		{
			switch (STATUS)
			{
			case 0:
				// Wait until the question is received
				if (USART1_RX_STA == 1)
				{
					USART1_RX_STA = 0;
					uint8_t lenth = strlen((char*) USART1_RX_BUF);
					if (!strcmp("exit\r\n", USART1_RX_BUF))
					{
						STATUS = 2;
						break;
					}
					// Set the answer
					ANSWER = *(USART1_RX_BUF + length - 1);
					// Send the question to respondent
					wifi_ap_send(USART1_RX_BUF, lenth - 2);
					memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF,
					USART1_MAX_RECV_LEN);
				}
				break;
			case 1:
				wifi_echo(1);
				break;
			case 2:
				// EXIT
				break;
			}
		}

	}
}

//空闲中断回调函数
void USAR_UART_IDLECallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART2)
	{
		if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE))
		{
			__HAL_UART_CLEAR_IDLEFLAG(huart);   //清空标志位
			HAL_UART_DMAStop(huart);            //停止DMA接收
			USART2_RX_STA = 1;                  //接收完成标志位
		}
	}

}

void UART1_IDLECallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1)
	{
		if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE))
		{
			__HAL_UART_CLEAR_IDLEFLAG(huart);   //清空标志位
			HAL_UART_DMAStop(huart);            //停止DMA接收
			USART1_RX_STA = 1;                  //接收完成标志位
		}
	}
}

