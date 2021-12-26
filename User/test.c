#include "test.h"
#include <stdarg.h>
#include <stdio.h>
#include "usart.h"
#include "wifi_config.h"
#include "lcd.h"
#include "string.h"
#include "stdlib.h"

uint16_t USART2_RX_STA = 0;
uint8_t USART2_TX_BUF[USART2_MAX_SEND_LEN];
uint8_t USART2_RX_BUF[USART2_MAX_RECV_LEN];

uint16_t USART1_RX_STA = 0;
uint8_t USART1_RX_BUF[USART1_MAX_RECV_LEN];

uint8_t TIMER_count = 0;
uint8_t TIMER_MAX_count = 0;
uint8_t countdown_flag = 0;

uint8_t STATUS;

char ANSWER[100];
char QUESTION[100];
char MAX_TIME[100];
char POINT[100];
char WHOLE_QUESTION[500];

extern TIM_HandleTypeDef htim3;

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
		// Respondent
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
		// Questioner
		else
		{
			switch (STATUS)
			{
			// Questioning mode
			case 0:
				wifi_echo(1);
				// Wait until the question is received
				if (USART1_RX_STA == 1 && countdown_flag == 0)
				{
					USART1_RX_STA = 0;
					if (!strcmp("exit\r\n", (char*)USART1_RX_BUF))
					{
						STATUS = 2;
						break;
					}
					// Set the 4 part of a whole question
					char delims[] = "#";
					char *result = NULL;
					result = strtok((char*)USART1_RX_BUF, delims);
					if(result != NULL) {
						strcpy(QUESTION, result);
						usart1_printf("%s\r\n", result);
						usart1_printf("%s\r\n", QUESTION);
					    result = strtok( NULL, delims );
					}
					if(result != NULL) {
						strcpy(POINT, result);
						usart1_printf("%s\r\n", result);
						usart1_printf("%s\r\n", POINT);
					    result = strtok( NULL, delims );
					}
					if(result != NULL) {
						strcpy(MAX_TIME, result);
						usart1_printf("%s\r\n", result);
						usart1_printf("%s\r\n", MAX_TIME);
					    result = strtok( NULL, delims );
					}
					if(result != NULL) {
						strcpy(ANSWER, result);
						usart1_printf("%s\r\n", result);
						usart1_printf("%s\r\n", ANSWER);
					}
					// ANSWER = *(USART1_RX_BUF + length - 1);
					// Send the question to respondent
					strncat(WHOLE_QUESTION, "[", 5);
					strncat(WHOLE_QUESTION, QUESTION, strlen(QUESTION)+5);
					strncat(WHOLE_QUESTION, "][", 5);
					strncat(WHOLE_QUESTION, POINT, strlen(POINT)+5);
					strncat(WHOLE_QUESTION, "][", 5);
					strncat(WHOLE_QUESTION, MAX_TIME, strlen(MAX_TIME)+5);
					strncat(WHOLE_QUESTION, "]", 5);
					usart1_printf("%s\r\n", WHOLE_QUESTION);
					uint8_t length = strlen(WHOLE_QUESTION);
					wifi_ap_send((uint8_t*)WHOLE_QUESTION, length);
					// Clear the buffer and refuse to read question from USART anymore
					HAL_UART_DMAStop(&huart1);
					memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
					// Start count down
					usart1_printf("[start count down]");
					TIMER_count = 0;
					TIMER_MAX_count = atoi(MAX_TIME); // 还需添加倒计时多少秒
					countdown_flag = 1;
					HAL_TIM_Base_Start_IT(&htim3);
					STATUS = 1;
				}
				break;
			// Answer-receiving mode
			case 1:
				wifi_echo(1);
				// Still counting down
				if (countdown_flag == 1) {

				}
				// Finish counting down
				else
				{
					STATUS = 0;
					usart1_printf("[count down over in case 1]");
				}
				break;
			// Judging mode
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

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	TIMER_count += 1;
	usart1_printf("%d\r\n", TIMER_count);
	wifi_ap_send(&TIMER_count, strlen((char*) TIMER_count));
	if (TIMER_count >= TIMER_MAX_count) {
		usart1_printf("[count down over]");
		countdown_flag = 0;
		TIMER_count = 0;
		TIMER_MAX_count = 0;
		// Send message to respondent
		uint8_t response_message[20] = "[Time Exceed Limit]";
		wifi_ap_send(response_message, strlen((char*) response_message));
		// Close the interrupt and open DMA receive
		HAL_TIM_Base_Stop_IT(&htim3);
		HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF, USART1_MAX_RECV_LEN);
	}
}

