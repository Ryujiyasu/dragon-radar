#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "ui/radar_view.h"

/* Wired to Type 2BP EVK Rev4.1 "Test pads for MCU":
 *   ESP32-P4 GPIO47 (RX)  <--  TP48 (QN9090 PIO_8 / USART0_TXD)   3.3V CMOS
 *   ESP32-P4 GPIO48 (TX)  -->  TP47 (QN9090 PIO_9 / USART0_RXD)   optional
 *   GND <-> GND
 * QN9090 BOARD_DEBUG_UART_BAUDRATE = 3 Mbps.
 * GPIO 47/48 chosen because they are physically adjacent on Waveshare 3.4C
 * 40-pin header right column (verified on IMG_0567 board photo, 5/19). */
#define UWB_UART_NUM            1
#define UWB_UART_BAUD           3000000
#define UWB_UART_RX_PIN         47
#define UWB_UART_TX_PIN         48
#define UWB_UART_RX_BUF_SIZE    2048

void uwb_uart_init(void);

/* Block until a valid RADAR measurement is parsed (status=0) or timeout. */
bool uwb_uart_recv(uwb_measurement_t *out, uint32_t timeout_ms);
