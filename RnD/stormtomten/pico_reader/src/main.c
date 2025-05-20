#include "config.h"
#include "hardware/watchdog.h"
#include "lwip/ip4_addr.h"
#include "lwip/tcp.h"
#include "mfrc522.h"
#include "network.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "secrets.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint8_t rec[MAX_LEN];
int state = 0;
bool message_pending = false;
bool awaiting_response = false;
bool response_ready = false;
struct tcp_pcb *client_pcb = NULL;
uint8_t msg_send[MAX_LEN];
uint8_t tagtype[MAX_LENGTH] = {0};
uint8_t uid[MAX_LENGTH] = {0};
uint8_t buf[MAX_LENGTH] = {0};
uint8_t uid_len = 0;
uint8_t block_index = 1;
uint8_t default_key_a[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
absolute_time_t connection_start_time;
const uint32_t CONNECTION_TIMEOUT_MS = 10000;

void watchdog_init_and_start() {
  watchdog_enable(WATCHDOG_TIMEOUT_MS, 1);
  printf("Watchdog timer started with timeout: %d\n", WATCHDOG_TIMEOUT_MS);
}

void watchdog_kick() {
  watchdog_update();
  // printf("Kick\n");
}

int get_uid() {
  uint8_t status = rfid_get_uid(uid, &uid_len);
  if (status == MI_OK) {
    printf("UID: ");
    for (uint8_t i = 0; i < 4; i++) {
      printf("%02X ", uid[i]);
    }
    printf("\n");
  } else {
    printf("Failed to get UID\n");
  }
}

int read_card() {
  memset(tagtype, 0, sizeof(tagtype[0]) * MAX_LENGTH);
  memset(uid, 0, sizeof(uid[0]) * MAX_LENGTH);
  memset(buf, 0, sizeof(buf[0]) * MAX_LENGTH);
  uid_len = 0;

  // REQ_WUPA == 0x52, wakeup halted cards && new card
  // REQ_REQA == 0x26, read only "new" cards
  uint8_t req_mode = REQ_WUPA;
  uint8_t status = rfid_request(req_mode, tagtype);

  if (status != MI_OK) {
    return 1;
    // printf("RFID timeout!\n");
  }

  if (tagtype[0] != 0x04 && tagtype[1] != 0x00) {
    printf("RFID tag type not allowed!\n");
    return 2;
  }

  status = rfid_anticoll(uid, &uid_len);
  if (status != MI_OK) {
    printf("RFID tag anti collision failed!\n");
    return 3;
  }

  // dump general card information
  uint8_t pad = 10;

  printf("%-*s: ", pad, "UID");
  for (int i = 0; i < 4; i++) {
    printf("%02X ", uid[i]);
  }
  printf("\n");
  return 0;
}

int main() {
  stdio_init_all();

  watchdog_init_and_start();

  if (wifi_connect(SSID, PASS) == 0) {
    printf("Wifi connected\n");
    printf("IP address: %s\n", get_ip());
  }

  rfid_init(
      // spi baud
      1 * 1000 * 1000,
      // spi inst *
      spi0,
      // sck, mosi, miso
      18, 19, 16,
      // cs, rst
      17, 20);

  printf("RFID is initialized\n");

  while (true) {
    cyw43_arch_poll(); // important for Wi-Fi driver

    if (!awaiting_response && !response_ready && client_pcb == NULL) {
      check_and_reconnect_wifi(SSID, PASS);
      if (read_card() == 0) {
        message_pending = true;
      }
    }
    if (message_pending) {
      send_tcp_message(uid);
      message_pending = false;
      awaiting_response = true;
    }
    if (response_ready) {
      printf("Server responded: %s\n", rec);
      response_ready = false;
    }

    // Check for connection timeout
    if (awaiting_response && client_pcb != NULL) {
      if (absolute_time_diff_us(connection_start_time, get_absolute_time()) /
              1000 >=
          CONNECTION_TIMEOUT_MS) {
        printf("TCP connection timed out\n");
        tcp_close(client_pcb);
        client_pcb = NULL;
        awaiting_response = false;
        message_pending = true;
      }
    }

    watchdog_kick();
    printf("Status: pending: %s waiting: %s ready: %s\n",
           message_pending ? "true" : "false",
           awaiting_response ? "true" : "false",
           response_ready ? "true" : "false");
    sleep_ms(800);
  }
}
