#include "test.h"
#include <stdarg.h>
#include <stdio.h>
#include "usart.h"
#include "wifi_config.h"
#include "lcd.h"
#include "string.h"
#include "stdlib.h"

uint8_t State;

uint16_t USART2_RX_STA = 0;
uint8_t USART2_TX_BUF[USART2_MAX_SEND_LEN];
uint8_t USART2_RX_BUF[USART2_MAX_RECV_LEN];

uint16_t USART1_RX_STA = 0;
uint8_t USART1_RX_BUF[USART1_MAX_RECV_LEN];

uint8_t TIMER_count = 0;

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
	State = 0;
	int total_score = 0;

	Question *question;

	Question questions[5] =
	{
	{ "Test Question A WERTYUIOPASDFGHJKLZXCVBNM\r\n", "Yes", "No", "IDK", "DUNJIAO", 'A', 20, 20 },
	{ "Test Question B WERTYUIOPASDFGHJKLZXCVBNM\r\n", "Yes", "No", "IDK", "DUNJIAO", 'B', 10, 15 },
	{ "Test Question C WERTYUIOPASDFGHJKLZXCVBNM\r\n", "Yes", "No", "IDK", "DUNJIAO", 'C', 15, 15 },
	{ "Test Question D WERTYUIOPASDFGHJKLZXCVBNM\r\n", "Yes", "No", "IDK", "DUNJIAO", 'D', 25, 10 },
	{ "Test Question E WERTYUIOPASDFGHJKLZXCVBNM\r\n", "Yes", "No", "IDK", "DUNJIAO", 'C', 10, 10 } };

	__HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);                        //启动空闲中断
	__HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
	HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF, USART1_MAX_RECV_LEN);
	HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF, USART2_MAX_RECV_LEN); //开启DMA接收
	uint8_t mode = 0;  //两个wifi模块配置的模式为0和1，连接后TCP客户端为透传，TCP服务器是根据端口进行数据发送
	wifi_init(mode);
	// Respondent
	if (mode)
	{
		// Show "RESPONDENT" on LCD
		LCD_Clear(WHITE);
		POINT_COLOR = BLACK;
		BACK_COLOR = WHITE;
		LCD_ShowString(10, 20, 200, 24, 24, (uint8_t*) "RESPONDENT");
		while (1)
		{
			if (USART2_RX_STA == 1)
			{
				usart1_printf("%s", USART2_RX_BUF);
				LCD_Fill(10, 80, 210, 104, WHITE);
				LCD_ShowString(10, 80, 200, 24, 16, USART2_RX_BUF);

				USART2_RX_STA = 0;
				memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN);
				HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF, USART2_MAX_RECV_LEN);
			}

			if (USART1_RX_STA == 1)
			{
				USART1_RX_STA = 0;
				usart2_printf("%s", USART1_RX_BUF);
				memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
				HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF, USART1_MAX_RECV_LEN);
			}
		}
	}
	// Questioner
	else
	{
		// Show "QUESTIONER" on LCD
		LCD_Clear(WHITE);
		POINT_COLOR = BLACK;
		BACK_COLOR = WHITE;
		LCD_ShowString(10, 20, 200, 24, 24, (uint8_t*) "QUESTIONER");
		while (1)
		{
			switch (State)
			{
			case 0:
				if (USART2_RX_STA == 1)
				{
					USART2_RX_STA = 0;
					memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF, USART2_MAX_RECV_LEN);
				}
				if (USART1_RX_STA == 1)
				{
					USART1_RX_STA = 0;
					memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF, USART1_MAX_RECV_LEN);
				}
				LCD_ShowString(10, 40, 200, 100, 24, (uint8_t*) "Please Choose One Question From 0-4.");
				State = 1;
				break;
			case 1:
				if (USART2_RX_STA == 1)
				{
					USART2_RX_STA = 0;
					memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF, USART2_MAX_RECV_LEN);
				}
				if (USART1_RX_STA == 1)
				{
					USART1_RX_STA = 0;
					usart1_printf("Choice Received\r\n");
					int idx = USART1_RX_BUF[0] - 48;
					if (idx < 0 || idx > 4)
					{
						LCD_ShowString(10, 150, 200, 100, 16, (uint8_t*) "Illegal.");
						memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
						HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF, USART1_MAX_RECV_LEN);
						break;
					}
					memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF, USART1_MAX_RECV_LEN);

					// TODO: Should be parsed from the input
					TIMER_count = question->time_limit;

					question = &(questions[idx]);
					uint8_t length = strlen((char*) question->desc);
					wifi_ap_send((uint8_t*) question->desc, length);

					// Show "Answer Receiving" on LCD
					LCD_Clear(WHITE);
					LCD_ShowString(10, 40, 200, 24, 24, (uint8_t*) "Answer Receiving");

					HAL_Delay(1000);
					HAL_TIM_Base_Start_IT(&htim3);

					// Change the state
					State = 2;
				}
				break;
			case 2:
				if (USART2_RX_STA == 1)
				{
					USART2_RX_STA = 0;

					char *received_message;
					char user_answer;
					strtok((char*) USART2_RX_BUF, ":");
					received_message = strtok(NULL, ":");
					user_answer = received_message[0];

					// Show the answer on the LCD
					LCD_ShowString(30, 65, 200, 24, 16, (uint8_t*) received_message);

					// Show contents through USART
					usart1_printf("Received message: %s\r\n", received_message);
					memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF, USART2_MAX_RECV_LEN);

					if (user_answer == question->correct_answer)
					{
						// Answer is correct
						// Stop the timer
						HAL_TIM_Base_Stop_IT(&htim3);

						// Send the earned score
						char correct_info[100];
						sprintf(correct_info, "[Answer Correct] [%d]\r\n", question->value);
						uint8_t length = strlen(correct_info);
						wifi_ap_send((uint8_t*) correct_info, length);
						total_score += question->value;

						HAL_Delay(1000);

						// Change State
						State = 3;
					}
					else
					{
						// Answer is incorrect
						// Send the incorrect score
						char *correct_info = "[Wrong Answer] [0]\r\n";
						uint8_t length = strlen(correct_info);
						wifi_ap_send((uint8_t*) correct_info, length);
					}

				}
				if (USART1_RX_STA == 1)
				{
					USART1_RX_STA = 0;
					memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF, USART1_MAX_RECV_LEN);
				}
				break;
			case 3:
				if (USART2_RX_STA == 1)
				{
					USART2_RX_STA = 0;
					memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF, USART2_MAX_RECV_LEN);
				}
				if (USART1_RX_STA == 1)
				{
					USART1_RX_STA = 0;
					memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF, USART1_MAX_RECV_LEN);
				}

				// Show "Judging..." on LCD
				LCD_Clear(WHITE);
				POINT_COLOR = BLACK;
				BACK_COLOR = WHITE;
				LCD_ShowString(10, 40, 200, 24, 24, (uint8_t*) "Judging...");

				// Send the total score
				char result[100];
				sprintf(result, "total score: %d\r\n", total_score);
				uint8_t length = strlen((char*) result);
				wifi_ap_send((uint8_t*) result, length);

				HAL_Delay(1000);

				// Ask for restart or not
				LCD_ShowString(10, 80, 200, 24, 16, (uint8_t*) "Restart? y/n");

				// Change the State
				State = 4;
				break;
			case 4:
				if (USART2_RX_STA == 1)
				{
					USART2_RX_STA = 0;
					memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF, USART2_MAX_RECV_LEN);
				}
				if (USART1_RX_STA == 1)
				{
					USART1_RX_STA = 0;
					char input = USART1_RX_BUF[0];
					if (input == 'y' || input == 'Y')
					{
						// Show "Waiting..." on LCD
						LCD_Clear(WHITE);
						POINT_COLOR = BLACK;
						BACK_COLOR = WHITE;
						LCD_ShowString(10, 40, 200, 24, 24, (uint8_t*) "Waiting...");

						State = 0;
					}
					else if (input == 'n' || input == 'N')
					{
						// Show "CLOSED" on LCD
						LCD_Clear(WHITE);
						POINT_COLOR = BLACK;
						BACK_COLOR = WHITE;
						LCD_ShowString(10, 40, 200, 24, 24, (uint8_t*) "CLOSED");

						State = 5;
					}
					else
					{
						usart1_printf("Illegal input, please input again.\r\n");
					}
					memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF, USART1_MAX_RECV_LEN);
				}
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

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	usart1_printf("%d\r\n", TIMER_count);
	if (TIMER_count == 0)
	{
		usart1_printf("[count down over]\r\n");
		char *counting_message = "[Time Exceed Limit]\r\n";
		uint8_t len = strlen(counting_message);
		wifi_ap_send((uint8_t*) counting_message, len);

		// Shut down the timer
		HAL_TIM_Base_Stop_IT(&htim3);

		// Set the state to judging mode
		HAL_Delay(1000);

		State = 3;
	}
	else
	{
		char counting_message[25];
		sprintf(counting_message, "time left: %d\r\n", TIMER_count);
		uint8_t len = strlen(counting_message);
		wifi_ap_send((uint8_t*) counting_message, len);
	}
	TIMER_count--;
}

