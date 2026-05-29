#include "uwb_uart.h"

#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"

#define LINE_BUF_SIZE   256

static const char *TAG = "uwb_uart";

static char s_line[LINE_BUF_SIZE];
static int  s_line_len;

void uwb_uart_init(void)
{
    const uart_config_t cfg = {
        .baud_rate  = UWB_UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(UWB_UART_NUM, UWB_UART_RX_BUF_SIZE, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UWB_UART_NUM, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(UWB_UART_NUM, UWB_UART_TX_PIN, UWB_UART_RX_PIN, -1, -1));
    s_line_len = 0;
    ESP_LOGI(TAG, "UART%d @ %d baud, RX=GPIO%d, TX=GPIO%d ready",
             UWB_UART_NUM, UWB_UART_BAUD, UWB_UART_RX_PIN, UWB_UART_TX_PIN);
}

/* Parse one line containing "RADAR,t=N,d=NN,az=I.F,el=I.F,st=N".
 * Q9.7 fractional bits restored to degrees as `int + sign(int) * frac/128`. */
static bool parse_radar_line(const char *line, uwb_measurement_t *out)
{
    const char *p = strstr(line, "RADAR,t=");
    if (!p) return false;

    int t, d, az_i, az_f, el_i, el_f, st;
    if (sscanf(p, "RADAR,t=%d,d=%d,az=%d.%d,el=%d.%d,st=%d",
               &t, &d, &az_i, &az_f, &el_i, &el_f, &st) != 7) return false;
    if (st != 0) return false;

    float az_deg = (float)az_i + ((az_i < 0) ? -(az_f / 128.0f) : (az_f / 128.0f));
    float el_deg = (float)el_i + ((el_i < 0) ? -(el_f / 128.0f) : (el_f / 128.0f));

    out->tag_id        = (uint8_t)(t + 1);            /* UWB meas index 0-based -> radar_view 1-based */
    out->distance_mm   = (uint16_t)(d * 10);          /* QN9090 reports cm */
    out->azimuth_deg   = (int16_t)az_deg;
    out->elevation_deg = (int16_t)el_deg;
    out->timestamp_ms  = (uint32_t)(esp_timer_get_time() / 1000);
    out->rssi          = 0;
    return true;
}

bool uwb_uart_recv(uwb_measurement_t *out, uint32_t timeout_ms)
{
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);

    while (xTaskGetTickCount() < deadline) {
        TickType_t remain = deadline - xTaskGetTickCount();
        uint8_t buf[64];
        int n = uart_read_bytes(UWB_UART_NUM, buf, sizeof(buf), remain);
        if (n <= 0) continue;

        for (int i = 0; i < n; i++) {
            uint8_t b = buf[i];
            if (b == '\n' || b == '\r') {
                if (s_line_len > 0) {
                    s_line[s_line_len] = '\0';
                    bool got = parse_radar_line(s_line, out);
                    s_line_len = 0;
                    if (got) return true;
                }
            } else if (s_line_len < LINE_BUF_SIZE - 1) {
                s_line[s_line_len++] = (char)b;
            } else {
                s_line_len = 0;  /* overflow -> drop line */
            }
        }
    }
    return false;
}
