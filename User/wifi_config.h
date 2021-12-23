#ifndef WIFI_CONFIG_H_
#define WIFI_CONFIG_H_
#include "main.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

uint8_t wifi_quit_trans(void);
void wifi_echo(uint8_t mode);
uint8_t wifi_send_cmd(char *cmd, char *ack, uint16_t waittime);
void wifi_ap_send(uint8_t *data_add, uint8_t len);
void wifi_init(uint8_t mode);
#endif /* WIFI_CONFIG_H_ */
