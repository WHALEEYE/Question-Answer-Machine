#include "wifi_config.h"
#include "test.h"
#include "usart.h"
//用户配置区
//连接端口号:8086,可自行修改为其他端口.
const char *portnum = "8086";

//8266的IP地址，不能修改
const char *ipnum = "192.168.4.1";

//WIFI STA模式,设置要去连接的路由器无线参数,请根据你自己的路由器设置,自行修改.
const char *wifista_ssid = "WIFI1234";			//路由器SSID号
const char *wifista_encryption = "wpawpa2_aes";	//wpa/wpa2 aes加密方式
const char *wifista_password = "1234567890"; 	//连接密码

//WIFI AP模式,模块对外的无线参数,可自行修改.
const char *wifiap_ssid = "WIFI1234";			//对外SSID号
const char *wifiap_encryption = "wpawpa2_aes";	//wpa/wpa2 aes加密方式
const char *wifiap_password = "1234567890"; 		//连接密码

//接收到的wifi模块数据进行串口打印回显
//mode:0，不清零USART2_RX_STA 1，清零USART2_RX_STA
void wifi_echo(uint8_t mode)
{
	if (USART2_RX_STA == 1)		//接收到一次数据了
	{
		usart1_printf("%s", USART2_RX_BUF);	//发送到串口
		if (mode)
		{
			USART2_RX_STA = 0;
			memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN);
			HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF, USART2_MAX_RECV_LEN);//开启下一次接收
		}
	}
}

//ATK-ESP8266退出透传模式
//返回值:0,退出成功;
//       1,退出失败
uint8_t wifi_quit_trans(void)
{
	while ((USART2->SR & 0X40) == 0)
		;	//等待发送空
	USART2->DR = '+';
	HAL_Delay(15);					//大于串口组帧时间(10ms)
	while ((USART2->SR & 0X40) == 0)
		;	//等待发送空
	USART2->DR = '+';
	HAL_Delay(15);					//大于串口组帧时间(10ms)
	while ((USART2->SR & 0X40) == 0)
		;	//等待发送空
	USART2->DR = '+';
	HAL_Delay(500);					//等待500ms
	return wifi_send_cmd("AT", "OK", 200);					//退出透传判断.
}

//ATK-ESP8266发送命令后,检测接收到的应答
//str:期待的应答结果
//返回值:0,没有得到期待的应答结果
//    其他,期待应答结果的位置(str的位置)
uint8_t* wifi_check_cmd(char *str)
{
	char *strx = 0;
	strx = strstr((const char*) USART2_RX_BUF, (const char*) str);
	return (uint8_t*) strx;
}

//向ATK-ESP8266发送命令
//cmd:发送的命令字符串
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:ms)
//返回值:0,发送成功(得到了期待的应答结果)
//       1,发送失败
uint8_t wifi_send_cmd(char *cmd, char *ack, uint16_t waittime)
{
	uint8_t *TheGET;
	unsigned int over_time = 0;
	memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN);
	for (int t = 0; t < 4; t++)					//收不到回复的情况下进行4次命令发送
	{
		usart2_printf("%s\r\n", cmd);	//发送命令
		TheGET = NULL;
		over_time = HAL_GetTick();
		while ((!TheGET) && ((over_time + waittime) > HAL_GetTick()))
		{
			if (USART2_RX_STA == 1)   //数据接收完成
			{
				USART2_RX_STA = 0;
				TheGET = wifi_check_cmd(ack);
				memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN); //清空接收缓存区
				HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF,
						USART2_MAX_RECV_LEN); //进行下一次接收
			}
		}
		if (TheGET != NULL)
		{
//			usart1_printf("ack:%s\r\n",(u8*)ack);
			return 0;
		}
	}
	return 1;
}

//向ATK-ESP8266发送指定数据
//data:发送的数据(不需要添加回车了)
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果)luojian
uint8_t wifi_send_data(char *data, char *ack, uint16_t waittime)
{
	uint8_t *TheGET;
	unsigned int over_time = 0;
	memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN);
	for (int t = 0; t < 4; t++)
	{
		usart2_printf("%s", data);	//发送数据
		TheGET = NULL;
		over_time = HAL_GetTick();
		while ((!TheGET) && ((over_time + waittime) > HAL_GetTick()))
		{
			if (USART2_RX_STA == 1)      //数据接收完成
			{
				USART2_RX_STA = 0;
				TheGET = wifi_check_cmd(ack);
				memset((char*) USART2_RX_BUF, 0, USART2_MAX_RECV_LEN); //清空接收缓存区
				HAL_UART_Receive_DMA(&huart2, USART2_RX_BUF,
						USART2_MAX_RECV_LEN);
			}
		}
		if (TheGET != NULL)
		{
//			usart1_printf("ack:%s\r\n",(u8*)ack);
			return 0;
		}
	}
	return 1;
}

//AP模式的数据发送
//data_add：数据的地址，len：数据的长度
void wifi_ap_send(uint8_t *data_add, uint8_t len)
{
	char *p;
	char cmd[16];
	p = (char*) data_add;
	sprintf((char*) cmd, "AT+CIPSEND=0,%d", len);  //0，网络连接ID号(0 ~ 4)
	wifi_send_cmd(cmd, "OK", 200);  //发送指定长度的数据
	wifi_send_data(p, "OK", 100);  //发送指定长度的数据
}

//ATK-ESP8266初始化
//mode：0，配置为AP模式+TCP服务器，提问者使用；1，配置为STA模式+TCP客户端，被提问者使用
void wifi_init(uint8_t mode)
{
	while (wifi_quit_trans())
		;
	while (wifi_send_cmd("AT", "OK", 200))		//检查WIFI模块是否在线
	{
		usart1_printf("wifi ununited\r\n");
		HAL_Delay(1000);
	}
	if (mode)
	{
		wifi_send_cmd("AT+CWMODE=1", "OK", 500);		//设置WIFI STA模式
		wifi_send_cmd("AT+RST", "OK", 200);            //复位重启(一个模块切换模式时需要进行复位)
		HAL_Delay(3000);
		wifi_send_cmd("ATE0", "OK", 200);
		char p[40];
		sprintf((char*) p, "AT+CWJAP=\"%s\",\"%s\"", wifista_ssid,
				wifista_password);            //设置无线参数:ssid,密码
		while (wifi_send_cmd(p, "WIFI GOT IP", 300))			//连接目标路由器,并且获得IP
		{
			usart1_printf("wifi connection fail\r\n");
			HAL_Delay(500);
		}
		while (wifi_send_cmd("AT+CIPMUX=0", "OK", 200))
			;
		sprintf((char*) p, "AT+CIPSTART=\"TCP\",\"%s\",%s", ipnum,
				(u8*) portnum);    //配置目标TCP服务器
		while (wifi_send_cmd(p, "OK", 200))
		{
			usart1_printf("config TCP fail\r\n");
			HAL_Delay(1000);
		}
		wifi_send_cmd("AT+CIPMODE=1", "OK", 200);         //透传
		wifi_send_cmd("AT+CIPSEND", "OK", 200);   		   //退出AT模式
		usart1_printf("wifi init is ok\r\n");
	}
	else
	{
		wifi_send_cmd("AT+CWMODE=2", "OK", 500);		//设置WIFI AP模式
		wifi_send_cmd("AT+RST", "OK", 200);            //复位重启(一个模块切换模式时需要进行复位)
		HAL_Delay(3000);
		wifi_send_cmd("ATE0", "OK", 200);
		char p[40];
		sprintf((char*) p, "AT+CWSAP=\"%s\",\"%s\",1,4", wifiap_ssid,
				wifiap_password);            //设置无线参数:ssid,密码
		while (wifi_send_cmd(p, "OK", 1000))
		{
			usart1_printf("wifi set fail\r\n");
			HAL_Delay(1000);
		}
		wifi_send_cmd("AT+CIPMUX=1", "OK", 200);   //0：单连接，1：多连接
		sprintf((char*) p, "AT+CIPSERVER=1,%s", (u8*) portnum);
		while (wifi_send_cmd(p, "OK", 200))     //开启Server模式，端口号为8086
		{
			usart1_printf("connection TCP fail\r\n");
			HAL_Delay(1000);
		}
		usart1_printf("wifi init is ok\r\n");
	}
}

