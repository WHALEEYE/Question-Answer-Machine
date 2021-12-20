#ifndef TEST_H_
#define TEST_H_
#include "main.h"





#define USART2_MAX_RECV_LEN		50					//最大接收缓存字节数
#define USART2_MAX_SEND_LEN		50					//最大发送缓存字节数

#define USART1_MAX_RECV_LEN     50

extern uint16_t USART2_RX_STA;
extern uint8_t  USART2_TX_BUF[USART2_MAX_SEND_LEN];
extern uint8_t  USART2_RX_BUF[USART2_MAX_RECV_LEN];



void usart1_printf(char *fmt,...);
void usart2_printf(char *fmt,...);
void Test(void);

#endif /* TEST_H_ */
