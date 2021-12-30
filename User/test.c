#include "test.h"
#include <stdarg.h>
#include <stdio.h>
#include "usart.h"
#include "wifi_config.h"
#include "lcd.h"
#include "string.h"
#include "stdlib.h"
#include "image.h"

QState State;

uint16_t USART2_RX_STA = 0;
uint8_t USART2_TX_BUF[USART2_MAX_SEND_LEN];
uint8_t USART2_RX_BUF[USART2_MAX_RECV_LEN];

uint16_t USART1_RX_STA = 0;
uint8_t USART1_RX_BUF[USART1_MAX_RECV_LEN];

int8_t TIMER_count = 0;

extern TIM_HandleTypeDef htim3;

uint16_t image_matrix[4096];
uint8_t image_cnt = 0;

const int send_segment = 32;

//串口打印函数
void usart1_printf(char *fmt, ...) {
	int i;
	char send_message[50];
	va_list ap;
	va_start(ap, fmt);
	i = vsprintf(send_message, fmt, ap);
	va_end(ap);
	if (i > 50) {
		return;
	}
	HAL_UART_Transmit(&huart1, (uint8_t*) send_message, i, 1000);
}

//串口打印函数
void usart2_printf(char *fmt, ...) {
	int i;
	char send_message[80];
	va_list ap;
	va_start(ap, fmt);
	i = vsprintf(send_message, fmt, ap);
	va_end(ap);
	if (i > 80) {
		return;
	}
	HAL_UART_Transmit(&huart2, (uint8_t*) send_message, i, 1000);
}

void Test(void) {
	State = 0;
	int total_score = 0;

	Question *question;

	Question questions[5] = { { "Test Question A DDDDDD", "Yes", "No", "IDK",
			"DUNJIAO", 'A', 20, 20, teacher }, { "Test Question B WWWWWW",
			"Yes", "No", "IDK", "DUNJIAO", 'B', 10, 15, NULL }, {
			"Test Question C RRRRRR", "Yes", "No", "IDK", "DUNJIAO", 'C', 15,
			15, NULL }, { "Test Question D NNNNNN", "Yes", "No", "IDK",
			"DUNJIAO", 'D', 25, 10, NULL }, { "Test Question E FFFFFF", "Yes",
			"No", "IDK", "DUNJIAO", 'C', 10, 10, NULL } };

	__HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);                        //启动空闲中断
	__HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
	HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF, USART1_MAX_RECV_LEN);
	HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF, USART2_MAX_RECV_LEN); //开启DMA接收

	uint8_t mode = 0;  //两个wifi模块配置的模式为0和1，连接后TCP客户端为透传，TCP服务器是根据端口进行数据发送

	wifi_init(mode);
	// Respondent
	if (mode) {

		// Show "RESPONDENT" on LCD
		LCD_Clear(WHITE);
		POINT_COLOR = BLACK;
		BACK_COLOR = WHITE;
		LCD_ShowString(10, 20, 200, 24, 24, (uint8_t*) "RESPONDENT");
		POINT_COLOR = RED;
		LCD_ShowString(10, 80, 200, 24, 24, (uint8_t*) "CS301");
		LCD_ShowString(10, 110, 200, 24, 24, (uint8_t*) "Question");
		LCD_ShowString(10, 140, 200, 24, 24, (uint8_t*) "Answer");
		LCD_ShowString(10, 170, 200, 24, 24, (uint8_t*) "Machine");
		POINT_COLOR = BLACK;
		LCD_ShowString(10, 280, 200, 24, 16, (uint8_t*) "Waiting For Question");

		while (1) {
			if (USART2_RX_STA == 1) {
				USART2_RX_STA = 0;
				char choice[50];
				switch (USART2_RX_BUF[0]) {
				case 'q':
					LCD_Clear(WHITE);
					LCD_ShowString(10, 20, 220, 90, 24,
							(uint8_t*) USART2_RX_BUF + 2);
					break;
				case 'a':
					sprintf(choice, "A. %s", USART2_RX_BUF + 2);
					LCD_ShowString(10, 120, 220, 24, 24, (uint8_t*) choice);
					break;
				case 'b':
					sprintf(choice, "B. %s", USART2_RX_BUF + 2);
					LCD_ShowString(10, 150, 220, 24, 24, (uint8_t*) choice);
					break;
				case 'c':
					sprintf(choice, "C. %s", USART2_RX_BUF + 2);
					LCD_ShowString(10, 180, 220, 24, 24, (uint8_t*) choice);
					break;
				case 'd':
					sprintf(choice, "D. %s", USART2_RX_BUF + 2);
					LCD_ShowString(10, 210, 220, 24, 24, (uint8_t*) choice);
					LCD_ShowString(10, 260, 120, 24, 24,
							(uint8_t*) "Time Left: ");
					break;
				case 't':
					// Each time a new timer count, refresh the info region
					LCD_Fill(10, 290, 230, 310, WHITE);
					// Refresh the timer count region
					LCD_Fill(140, 260, 180, 290, WHITE);
					LCD_ShowString(140, 260, 40, 24, 24,
							(uint8_t*) USART2_RX_BUF + 2);
					break;
				case 'i':
					LCD_Fill(10, 290, 230, 310, WHITE);
					LCD_ShowString(10, 290, 220, 16, 16,
							(uint8_t*) USART2_RX_BUF + 2);
					break;
				case 'M':
					LCD_Fill(10, 110, 230, 310, WHITE);
					LCD_ShowString(10, 120, 220, 16, 16,
							(uint8_t*) USART2_RX_BUF + 2);
					POINT_COLOR = RED;
					LCD_ShowString(10, 140, 220, 24, 24,
							(uint8_t*) "Total Score");
					POINT_COLOR = BLACK;
					break;
				case 'S':
					POINT_COLOR = RED;
					LCD_ShowString(10, 170, 220, 24, 24,
							(uint8_t*) USART2_RX_BUF + 2);
					POINT_COLOR = BLACK;
					LCD_ShowString(10, 280, 200, 24, 16,
							(uint8_t*) "Waiting For Question");
					break;
				case 'p':
					usart1_printf("receive p: %d\r\n", image_cnt);
					memcpy(image_matrix + send_segment * image_cnt, USART2_RX_BUF + 2, send_segment);

					image_cnt++;
					break;
				case 'P':
					usart1_printf("receive P: %d\r\n", image_cnt);
					memcpy(image_matrix + send_segment * image_cnt, USART2_RX_BUF + 2, send_segment);

					image_cnt = 0;
					LCD_ShowImage(image_matrix, 10, 60, 64, 64);
					break;
				case 'C':
					LCD_Clear(WHITE);
					POINT_COLOR = BLACK;
					BACK_COLOR = WHITE;
					LCD_ShowString(10, 20, 200, 24, 24,
							(uint8_t*) "RESPONDENT");
					POINT_COLOR = RED;
					LCD_ShowString(10, 80, 200, 24, 24, (uint8_t*) "CS301");
					LCD_ShowString(10, 110, 200, 24, 24, (uint8_t*) "Question");
					LCD_ShowString(10, 140, 200, 24, 24, (uint8_t*) "Answer");
					LCD_ShowString(10, 170, 200, 24, 24, (uint8_t*) "Machine");
					POINT_COLOR = BLACK;
					LCD_ShowString(10, 270, 200, 24, 24, (uint8_t*) "CLOSED");
					break;
				}
				memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN);
				HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF,
				USART2_MAX_RECV_LEN);
			}

			if (USART1_RX_STA == 1) {
				USART1_RX_STA = 0;
				usart2_printf("%s", USART1_RX_BUF);
				memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
				HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF,
				USART1_MAX_RECV_LEN);
			}
		}
	}
	// Questioner
	else {
		// Show "QUESTIONER" on LCD
		LCD_Clear(WHITE);
		POINT_COLOR = BLACK;
		BACK_COLOR = WHITE;
		LCD_ShowString(10, 20, 200, 24, 24, (uint8_t*) "QUESTIONER");
		while (1) {
			switch (State) {
			case 0:
				if (USART2_RX_STA == 1) {
					USART2_RX_STA = 0;
					memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF,
					USART2_MAX_RECV_LEN);
				}
				if (USART1_RX_STA == 1) {
					USART1_RX_STA = 0;
					memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF,
					USART1_MAX_RECV_LEN);
				}
				LCD_ShowString(10, 40, 220, 100, 24,
						(uint8_t*) "Please Choose One Question From 0-4.");
				State = 1;
				break;
			case 1:
				if (USART2_RX_STA == 1) {
					USART2_RX_STA = 0;
					memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF,
					USART2_MAX_RECV_LEN);
				}
				if (USART1_RX_STA == 1) {
					USART1_RX_STA = 0;
					int idx = USART1_RX_BUF[0] - 48;
					if (idx < 0 || idx > 4) {
						LCD_ShowString(10, 150, 200, 100, 16,
								(uint8_t*) "Illegal.");
						memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
						HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF,
						USART1_MAX_RECV_LEN);
						break;
					}
					memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF,
					USART1_MAX_RECV_LEN);

					question = &(questions[idx]);

					TIMER_count = question->time_limit;

					char question_str[50];
					sprintf(question_str, "q %s [%d] [%d s]", question->desc,
							question->value, question->time_limit);
					uint8_t length = strlen(question_str);
					wifi_ap_send((uint8_t*) question_str, length);

					// Refresh the info region
					LCD_Fill(10, 290, 230, 310, WHITE);
					// Show info "Question Sent" on LCD
					LCD_ShowString(10, 290, 220, 16, 16,
							(uint8_t*) "Question Sent.");
					HAL_Delay(100);

					char choice_str[80];
					sprintf(choice_str, "a %s", question->choice1);
					length = strlen(choice_str);
					wifi_ap_send((uint8_t*) choice_str, length);
					HAL_Delay(50);

					sprintf(choice_str, "b %s", question->choice2);
					length = strlen(choice_str);
					wifi_ap_send((uint8_t*) choice_str, length);
					HAL_Delay(50);

					sprintf(choice_str, "c %s", question->choice3);
					length = strlen(choice_str);
					wifi_ap_send((uint8_t*) choice_str, length);
					HAL_Delay(50);

					sprintf(choice_str, "d %s", question->choice4);
					length = strlen(choice_str);
					wifi_ap_send((uint8_t*) choice_str, length);
					HAL_Delay(50);

					if (question->image != NULL) {

						for (int i = 0; i < 4096/send_segment; i++) {
							if (i != 4096/send_segment - 1) {
								choice_str[0] = 'p';
								usart1_printf("send p: %d\r\n", i);
							} else {
								choice_str[0] = 'P';
								usart1_printf("send P: %d\r\n", i);
							}
							choice_str[1] = ' ';
							memcpy(choice_str + 2, question->image+(i*send_segment), send_segment);
							usart1_printf("memcpy");
							for (int j=0;j<send_segment+2;j++){
								usart1_printf("choice_str+%d: %x",j, choice_str[j]);
							}
							wifi_ap_send((uint8_t*) choice_str, send_segment+2);
							usart1_printf("sending");
							HAL_Delay(50);
						}
					}

					// Refresh the display region
					LCD_Fill(10, 50, 230, 310, WHITE);
					// Show the Question on LCD
					LCD_ShowString(10, 60, 220, 90, 24,
							(uint8_t*) question_str + 2);
					// Show "Answer Receiving" on LCD
					LCD_Clear(WHITE);
					LCD_ShowString(10, 40, 200, 24, 24,
							(uint8_t*) "Answer Receiving");

					HAL_TIM_Base_Start_IT(&htim3);

					// Change the state
					State = RECVING;
				}
				break;
			case 2:
				if (USART2_RX_STA == 1) {
					USART2_RX_STA = 0;

					char *received_message;
					char user_answer;
					strtok((char*) USART2_RX_BUF, ":");
					received_message = strtok(NULL, ":");
					user_answer = received_message[0];

//					// Show the answer on the LCD
//					LCD_ShowString(30, 65, 200, 24, 16, (uint8_t*) received_message);

//					// Show contents through USART
//					usart1_printf("Received message: %s\r\n", received_message);
//					memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN);
//					HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF, USART2_MAX_RECV_LEN);

					if (user_answer == question->correct_answer) {
						// Answer is correct
						// Stop the timer
						HAL_TIM_Base_Stop_IT(&htim3);

						// Send the earned score
						char correct_info[50];
						sprintf(correct_info, "M [Answer Correct] [%d]",
								question->value);
						uint8_t length = strlen(correct_info);
						wifi_ap_send((uint8_t*) correct_info, length);
						total_score += question->value;

						// Refresh the status region
						LCD_Fill(10, 160, 230, 310, WHITE);
						// Show "Judging" on LCD
						POINT_COLOR = RED;
						LCD_ShowString(10, 160, 220, 24, 24,
								(uint8_t*) "Judging");
						LCD_ShowString(10, 200, 220, 24, 24,
								(uint8_t*) "Result: Correct");
						POINT_COLOR = BLACK;

						// Change State
						State = 3;
					} else {
						// Answer is incorrect

						// Refresh the judge region
						LCD_Fill(10, 230, 230, 290, WHITE);
						// Show received answer on LCD
						char received_ans[20];
						sprintf(received_ans, "Received: %c", user_answer);
						LCD_ShowString(10, 230, 220, 24, 24,
								(uint8_t*) received_ans);
						LCD_ShowString(10, 260, 220, 24, 24,
								(uint8_t*) "Result: Wrong");

						// Send the incorrect score
						char *incorrect_info = "i [Wrong Answer] [0]";
						uint8_t length = strlen(incorrect_info);
						wifi_ap_send((uint8_t*) incorrect_info, length);
					}
				}
				if (USART1_RX_STA == 1) {
					USART1_RX_STA = 0;
					memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF,
					USART1_MAX_RECV_LEN);
				}
				break;
			case 3:
				if (USART2_RX_STA == 1) {
					USART2_RX_STA = 0;
					memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF,
					USART2_MAX_RECV_LEN);
				}
				if (USART1_RX_STA == 1) {
					USART1_RX_STA = 0;
					memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF,
					USART1_MAX_RECV_LEN);
				}

				// Send the total score
				char result[50];
				sprintf(result, "S %d", total_score);
				uint8_t length = strlen((char*) result);
				wifi_ap_send((uint8_t*) result, length);

				// Ask for restart or not
				LCD_ShowString(10, 250, 220, 24, 24,
						(uint8_t*) "Restart? [y/n]");

				// Change the State
				State = FINISHED;
				break;
			case 4:
				if (USART2_RX_STA == 1) {
					USART2_RX_STA = 0;
					memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF,
					USART2_MAX_RECV_LEN);
				}
				if (USART1_RX_STA == 1) {
					USART1_RX_STA = 0;
					char input = USART1_RX_BUF[0];
					if (input == 'y' || input == 'Y') {
						// Show "Waiting..." on LCD
						LCD_Clear(WHITE);
						POINT_COLOR = BLACK;
						BACK_COLOR = WHITE;
						LCD_ShowString(10, 40, 200, 24, 24,
								(uint8_t*) "Waiting...");

						State = 0;
					} else if (input == 'n' || input == 'N') {
						// Show "CLOSED" on LCD
						LCD_Clear(WHITE);
						POINT_COLOR = BLACK;
						BACK_COLOR = WHITE;
						LCD_ShowString(10, 40, 200, 24, 24,
								(uint8_t*) "CLOSED");

						State = 5;
					} else {
						usart1_printf("Illegal input, please input again.\r\n");
					}
					memset((char*) USART1_RX_BUF, 0, USART1_MAX_RECV_LEN);
					HAL_UART_Receive_DMA(&huart1, USART1_RX_BUF,
					USART1_MAX_RECV_LEN);
				}
				break;
			}
		}
	}
}

//空闲中断回调函数
void USAR_UART_IDLECallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART2) {
		if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE)) {
			__HAL_UART_CLEAR_IDLEFLAG(huart);   //清空标志位
			HAL_UART_DMAStop(huart);            //停止DMA接收
			USART2_RX_STA = 1;                  //接收完成标志位
		}
	}

}

void UART1_IDLECallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART1) {
		if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE)) {
			__HAL_UART_CLEAR_IDLEFLAG(huart);   //清空标志位
			HAL_UART_DMAStop(huart);            //停止DMA接收
			USART1_RX_STA = 1;                  //接收完成标志位
		}
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (TIMER_count == 0) {
		char *counting_message = "M [Time Exceed Limit]";
		uint8_t len = strlen(counting_message);
		wifi_ap_send((uint8_t*) counting_message, len);

		// Shut down the timer
		HAL_TIM_Base_Stop_IT(&htim3);

		// Refresh the status region
		LCD_Fill(10, 160, 230, 310, WHITE);
		// Show "Judging" on LCD
		POINT_COLOR = RED;
		LCD_ShowString(10, 160, 220, 24, 24, (uint8_t*) "Judging");
		LCD_ShowString(10, 200, 220, 24, 24, (uint8_t*) "Result: TLE");
		POINT_COLOR = BLACK;

		State = 3;
	} else {
		char counting_message[25];
		sprintf(counting_message, "t %d", TIMER_count);
		uint8_t len = strlen(counting_message);
		wifi_ap_send((uint8_t*) counting_message, len);

		// Each time a new timer count, refresh the judge region
		LCD_Fill(10, 230, 230, 290, WHITE);
		// Refresh the timer count region
		LCD_Fill(140, 190, 180, 220, WHITE);
		LCD_ShowString(140, 190, 40, 24, 24, (uint8_t*) counting_message + 2);
	}
	TIMER_count--;
}

