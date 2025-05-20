#ifndef NETWORK_H
#define NETWORK_H

#include "config.h"
#include "lwip/err.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
// #include <cstdint>
#include <stdbool.h>
#include <stdint.h>

extern bool message_pending;
extern bool awaiting_response;
extern bool response_ready;

extern struct tcp_pcb *client_pcb;
extern uint8_t msg_send[];
extern uint8_t rec[];
extern uint8_t uid[];

int wifi_connect(const char *ssid, const char *pass);
bool wifi_is_connected();
int check_and_reconnect_wifi(const char *ssid, const char *pass);
err_t send_tcp_message(uint8_t *msg);
err_t on_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
const char *get_ip();

#endif
