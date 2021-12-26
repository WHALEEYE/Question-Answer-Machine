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

int new_score = 0;
int total_score = 0;

int END_GAME_FLAG = 0;

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
	wifi_echo(1);
	while (1)
	{
		if (END_GAME_FLAG)
		{
			break;
		}
		// Respondent
		if (mode)
		{
			wifi_echo(1);
			if (USART1_RX_STA == 1)
			{
				USART1_RX_STA = 0;
				usart2_printf("%s", USART1_RX_BUF);
				memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
				HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF,
				USART1_MAX_RECV_LEN);
			}
		}
		// Questioner
		else
		{
			switch (STATUS)
			{
			// Questioning mode
			case 0:
				// Wait until the question is received
				if (USART1_RX_STA == 1 && countdown_flag == 0)
				{
					USART1_RX_STA = 0;
					if (!strcmp("exit\r\n", (char*) USART1_RX_BUF))
					{
						STATUS = 2;
						break;
					}

					// Read 4 part of a whole question
					char delims1[] = "#";
					char delims2[] = "\r\n";
					char *result = NULL;
					result = strtok((char*) USART1_RX_BUF, delims1);
					if (result != NULL)
					{
						strcpy(QUESTION, result);
						usart1_printf("%s\r\n", QUESTION);
						result = strtok( NULL, delims1);
					}
					if (result != NULL)
					{
						strcpy(POINT, result);
						usart1_printf("%s\r\n", POINT);
						result = strtok( NULL, delims1);
					}
					if (result != NULL)
					{
						strcpy(MAX_TIME, result);
						usart1_printf("%s\r\n", MAX_TIME);
						result = strtok( NULL, delims2);
					}
					if (result != NULL)
					{
						strcpy(ANSWER, result);
						usart1_printf("%s\r\n", ANSWER);
					}

					// Send the question to respondent
					char WHOLE_QUESTION[500];
					strncat(WHOLE_QUESTION, "[", 5);
					strncat(WHOLE_QUESTION, QUESTION, strlen(QUESTION) + 5);
					strncat(WHOLE_QUESTION, "][", 5);
					strncat(WHOLE_QUESTION, POINT, strlen(POINT) + 5);
					strncat(WHOLE_QUESTION, "][", 5);
					strncat(WHOLE_QUESTION, MAX_TIME, strlen(MAX_TIME) + 5);
					strncat(WHOLE_QUESTION, "]\r\n\0", 10);
					usart1_printf("%s", WHOLE_QUESTION);
					uint8_t length = strlen(WHOLE_QUESTION);
					wifi_ap_send((uint8_t*) WHOLE_QUESTION, length);

					// Clear the buffer and refuse to read question from USART anymore
					HAL_UART_DMAStop(&huart1);
					memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
					memset(WHOLE_QUESTION, 0, length);

					// Start count down
					usart1_printf("[start count down]\r\n");
					char countdown_start_message[500] = "[start count down]\r\n";
					wifi_ap_send((uint8_t*) countdown_start_message,
							strlen(countdown_start_message));
					TIMER_count = 0;
					TIMER_MAX_count = atoi(MAX_TIME);
					countdown_flag = 1;
					HAL_TIM_Base_Start_IT(&htim3);
					STATUS = 1;
				}
				break;
				// Answer-receiving mode
			case 1:
				// Still counting down
				if (countdown_flag == 1)
				{
					if (USART2_RX_STA == 1)
					{
						usart1_printf("Answer Receive : %s", USART2_RX_BUF);
						char *recieve_answer = NULL;
						char delims3[] = "\r\n";
						recieve_answer = strtok((char*) USART2_RX_BUF, delims3);
						usart1_printf("%s\r\n", recieve_answer);
						// Answer is correct
						if (strcmp(ANSWER, recieve_answer))
						{
							// add new score
							new_score = atoi(POINT);

							// report to respondent
							char answer_correct_message[500] =
									"[Answer Correct] [Points awarded : ";
							strncat(answer_correct_message, POINT, 50);
							strncat(answer_correct_message, "]\r\n\0", 10);
							usart1_printf("%s", answer_correct_message);
							wifi_ap_send((uint8_t*) answer_correct_message,
									strlen(answer_correct_message));

							// clear DMA2 cache and continue receiving
							USART2_RX_STA = 0;
							memset((char*) USART2_RX_BUF, 0,
							USART2_MAX_RECV_LEN);
							HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF,
							USART2_MAX_RECV_LEN);

							// stop the count down immediately and enter the judging mode
							usart1_printf("[count down over]\r\n");
							countdown_flag = 0;
							TIMER_count = 0;
							TIMER_MAX_count = 0;
							HAL_TIM_Base_Stop_IT(&htim3);
							STATUS = 2;
						}
						// Answer is incorrect
						else
						{
							// add new score
							new_score = 0;

							// report to respondent
							char answer_incorrect_message[500] =
									"[Wrong Answer][Points awarded : 0]\r\n";
							usart1_printf("%s", answer_incorrect_message);
							wifi_ap_send((uint8_t*) answer_incorrect_message,
									strlen(answer_incorrect_message));

							// clear DMA2 cache and continue receiving
							USART2_RX_STA = 0;
							memset((char*) USART2_RX_BUF, 0,
							USART2_MAX_RECV_LEN);
							HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF,
							USART2_MAX_RECV_LEN);
						}
					}
				}
				// Finish counting down
				else
				{
					// Send message to respondent
					char countdown_finish_message[50] =
							"[Time Exceed Limit]\r\n";
					usart1_printf("%s", countdown_finish_message);
					wifi_ap_send((uint8_t*) countdown_finish_message,
							strlen(countdown_finish_message));

					// clear DMA2 cache and continue receiving
					USART2_RX_STA = 0;
					memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF,
					USART2_MAX_RECV_LEN);

					// enter the judging mode
					STATUS = 2;
				}
				break;
				// Judging mode
			case 2:
				// EXIT
				// 目前DMA1请求处于关闭状态

				// Respondent’s awarded point on this specific question should be added to total points
				total_score += new_score;

				// Display respondent’s current total mark: [Current total mark]
				char total_score_str[25];
				itoa(total_score, total_score_str, 10);
				char total_mark_message[500] = "[Current total mark : ";
				strncat(total_mark_message, total_score_str, 50);
				strncat(total_mark_message, "]\r\n\0", 10);
				usart1_printf("%s", total_mark_message);
				wifi_ap_send((uint8_t*) total_mark_message,
						strlen(total_mark_message));

				// Choose to either enter questioning mode again for next question or exit.
				// Display information: [Next Question]/[End the round]
				char continue_or_end_message[100] =
						"[Next Question]/[End the round] (input y/n)\r\n";
				usart1_printf("%s", continue_or_end_message);
				wifi_ap_send((uint8_t*) continue_or_end_message,
						strlen(continue_or_end_message));

				// Choose Continue
				if (strcmp("y\r\n", (char*) USART2_RX_BUF))
				{
					char continue_message[100] = "[Another Round Started]\r\n";
					usart1_printf("%s", continue_message);
					wifi_ap_send((uint8_t*) continue_message,
							strlen(continue_message));

					// DMA1 start receiving again
					HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF,
					USART1_MAX_RECV_LEN);

					// Back to Questioning Mode
					STATUS = 0;
				}
				// Choose End
				else
				{
					char end_message[100] =
							"[The Game is totally end !]\r\n[Your Final Score is : ";
					strncat(end_message, total_score_str, 50);
					strncat(end_message, "]\r\n\0", 10);
					usart1_printf("%s", end_message);
					wifi_ap_send((uint8_t*) end_message, strlen(end_message));

					// Close everything
					HAL_UART_DMAStop(&huart1);
					HAL_UART_DMAStop(&huart2);

					END_GAME_FLAG = 1;
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
	TIMER_count += 1;
	usart1_printf("%d\r\n", TIMER_count);
	char counting_message[25];
	itoa(TIMER_count, counting_message, 10);
	strncat(counting_message, "\r\n\0", 10);
	wifi_ap_send((uint8_t*) counting_message, strlen(counting_message));
	if (TIMER_count >= TIMER_MAX_count)
	{
		usart1_printf("[count down over]\r\n");
		countdown_flag = 0;
		TIMER_count = 0;
		TIMER_MAX_count = 0;
		// Close the interrupt and open DMA receive
		HAL_TIM_Base_Stop_IT(&htim3);
	}
}

