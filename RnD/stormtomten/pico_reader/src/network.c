#include "network.h"
#include "config.h"
#include "mfrc522.h"
#include <stdint.h>
#include <stdio.h>

extern uint8_t rec[MAX_LEN];
extern uint8_t msg_send[MAX_LEN];
extern uint8_t uid[MAX_LENGTH];
extern absolute_time_t connection_start_time;

int wifi_connect(const char *ssid, const char *pass) {
  if (cyw43_arch_init_with_country(CYW43_COUNTRY_SWEDEN)) {
    printf("Failed to initialize\n");
    return 1;
  }
  printf("Initialised\n");

  cyw43_arch_enable_sta_mode();

  if (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK,
                                         10000)) {
    printf("Failed to connect\n");
    return 2;
  }
  return 0;
}

bool wifi_is_connected() {
  int status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);

  if (status < 0) {
    return false;
  }

  if (status == CYW43_LINK_UP || status == CYW43_LINK_JOIN) {
    ip4_addr_t ip = cyw43_state.netif[CYW43_ITF_STA].ip_addr;
    return !ip4_addr_isany_val(ip);
  }
  return false;
}

int check_and_reconnect_wifi(const char *ssid, const char *pass) {
  if (!wifi_is_connected()) {
    printf("Wi-Fi disconnected, attempting to reconnect...\n");

    if (client_pcb != NULL) {
      tcp_close(client_pcb);
      client_pcb = NULL;
      printf("Closed old TCP-connection.\n");
    }
    sleep_ms(100);

    cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
    sleep_ms(100);

    if (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK,
                                           10000)) {
      printf("Failed to connect\n");
      return 1;
    }

    printf("Reconnected to Wi-Fi\n");
    printf("IP adress: %s\n",
           ip4addr_ntoa(&cyw43_state.netif[CYW43_ITF_STA].ip_addr));
    return 0;
  }
  return 0;
}

void on_error(void *arg, err_t err) {
  printf("TCP error: %d\n", err);
  if (client_pcb != NULL) {
    tcp_close(client_pcb);
    client_pcb = NULL;
  }
  awaiting_response = false;
  message_pending = true;
}
err_t on_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
  if (!p) {
    printf("Connection closed by peer or error\n");
    tcp_close(tpcb);
    client_pcb = NULL;
    awaiting_response = false;
    return ERR_OK;
  }

  int len = p->len < sizeof(rec) - 1 ? p->len : sizeof(rec) - 1;
  memcpy(rec, p->payload, len);
  rec[len] = '\0';
  response_ready = true;
  awaiting_response = false;
  tcp_close(tpcb);
  client_pcb = NULL;

  pbuf_free(p);
  return ERR_OK;
}

err_t on_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
  if (err != ERR_OK) {
    printf("TCP connect error: %d\n", err);
    tcp_close(tpcb);
    client_pcb = NULL;
    return err;
  }

  tcp_recv(tpcb, on_recv);

  err_t wr_err =
      tcp_write(tpcb, msg_send, strlen(msg_send), TCP_WRITE_FLAG_COPY);
  if (wr_err != ERR_OK) {
    printf("TCP write error: %d\n", wr_err);
    tcp_close(tpcb);
    client_pcb = NULL;
  } else {
    printf("Message sent: %s\n", msg_send);
    for (int i = 0; i < 4; i++) {
      printf("%02X ", msg_send[i]);
    }
  }

  return ERR_OK;
}

err_t send_tcp_message(uint8_t *msg) {
  if (client_pcb != NULL) {
    return ERR_INPROGRESS; // already in progress
  }

  ip4_addr_t addr;
  if (!ip4addr_aton(SERVER_IP, &addr)) {
    printf("Invalid server IP\n");
    return ERR_RTE;
  }

  client_pcb = tcp_new();
  if (!client_pcb) {
    printf("Failed to create TCP PCB\n");
    return ERR_VAL;
  }

  strncpy(msg_send, msg, sizeof(msg_send));
  connection_start_time = get_absolute_time();
  tcp_err(client_pcb, on_error);
  tcp_connect(client_pcb, &addr, SERVER_PORT, on_connected);
  return ERR_OK;
}

const char *get_ip() {
  return ip4addr_ntoa(&cyw43_state.netif[CYW43_ITF_STA].ip_addr);
}
