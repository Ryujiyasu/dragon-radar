#include "imu_bno086.h"

#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "imu_bno086";

#define BNO_ADDR        0x4B
#define BNO_I2C_HZ      100000      /* conservative; shared bus, low data rate */

/* SHTP channels */
#define CH_CMD          0
#define CH_EXEC         1
#define CH_CONTROL      2
#define CH_REPORTS      3

/* SH-2 report IDs */
#define SET_FEATURE_COMMAND             0xFD
#define REPORTID_ROTATION_VECTOR        0x05    /* absolute (uses magnetometer / true-ish north) */
#define REPORTID_GAME_ROTATION_VECTOR   0x08    /* relative yaw, no magnetometer (robust near magnets) */

/* Game Rotation Vector by default: immune to the USB/speaker magnets near the IMU,
 * and rotation-stability only needs *relative* yaw. Switch to ROTATION_VECTOR for
 * magnetic-north referenced heading (needs figure-8 mag calibration). */
#ifndef IMU_FEATURE_ID
#define IMU_FEATURE_ID  REPORTID_GAME_ROTATION_VECTOR
#endif

#define REPORT_INTERVAL_US  20000   /* 50 Hz */

static i2c_master_dev_handle_t s_dev = NULL;
static volatile bool  s_ok  = false;
static volatile float s_yaw = 0.0f;
static uint8_t s_seq[6] = {0};

/* Send one SHTP packet: 4-byte header + payload. */
static esp_err_t shtp_send(uint8_t channel, const uint8_t *data, uint16_t len)
{
    uint8_t pkt[24];
    uint16_t total = len + 4;
    if (total > sizeof(pkt)) return ESP_ERR_INVALID_SIZE;
    pkt[0] = total & 0xFF;
    pkt[1] = (total >> 8) & 0xFF;
    pkt[2] = channel;
    pkt[3] = s_seq[channel]++;
    memcpy(pkt + 4, data, len);
    return i2c_master_transmit(s_dev, pkt, total, 100);
}

/* SH-2 "Set Feature Command" -> enable a periodic sensor report. */
static esp_err_t enable_feature(uint8_t report_id, uint32_t interval_us)
{
    uint8_t p[17] = {0};
    p[0] = SET_FEATURE_COMMAND;
    p[1] = report_id;
    /* p[2] flags, p[3..4] change sensitivity = 0 */
    p[5] = interval_us & 0xFF;
    p[6] = (interval_us >> 8) & 0xFF;
    p[7] = (interval_us >> 16) & 0xFF;
    p[8] = (interval_us >> 24) & 0xFF;
    /* p[9..12] batch interval, p[13..16] sensor config = 0 */
    return shtp_send(CH_CONTROL, p, sizeof(p));
}

/* Read one SHTP packet (SparkFun-style: header re-sent per read transaction).
 * Stores up to cap payload bytes into out. Returns false on I2C error / no data. */
static bool read_packet(uint8_t *out, int cap, int *payload_len, uint8_t *channel)
{
    uint8_t hdr[4];
    if (i2c_master_receive(s_dev, hdr, 4, 100) != ESP_OK) return false;
    uint16_t len = (uint16_t)(hdr[0] | (hdr[1] << 8)) & 0x7FFF;   /* strip continuation bit */
    *channel = hdr[2];
    if (len < 4 || len > 1024) { *payload_len = 0; return true; } /* 0 / 0xFFFF => no data */

    int remaining = len - 4;
    *payload_len = remaining;
    int copied = 0;
    uint8_t tmp[68];
    while (remaining > 0) {
        int chunk = remaining > 60 ? 60 : remaining;
        if (i2c_master_receive(s_dev, tmp, chunk + 4, 100) != ESP_OK) return false;
        /* tmp[0..3] is the re-sent header; payload continues at tmp[4]. */
        int space = cap - copied;
        if (space > 0) {
            int n = chunk < space ? chunk : space;
            memcpy(out + copied, tmp + 4, n);
            copied += n;
        }
        remaining -= chunk;
    }
    return true;
}

/* Parse a rotation-vector report's quaternion (Q14) -> yaw degrees. */
static bool parse_rotation_vector(const uint8_t *payload, int plen, float *yaw_out)
{
    int idx = 0;
    if (plen > 0 && payload[0] == 0xFB) idx = 5;   /* skip 0xFB timebase + 4-byte delta */
    if (idx + 12 > plen) return false;
    if (payload[idx] != IMU_FEATURE_ID) return false;

    int16_t qi = (int16_t)(payload[idx + 4] | (payload[idx + 5] << 8));
    int16_t qj = (int16_t)(payload[idx + 6] | (payload[idx + 7] << 8));
    int16_t qk = (int16_t)(payload[idx + 8] | (payload[idx + 9] << 8));
    int16_t qr = (int16_t)(payload[idx + 10] | (payload[idx + 11] << 8));

    const float Q = 1.0f / 16384.0f;            /* Q14 fixed point */
    float i = qi * Q, j = qj * Q, k = qk * Q, r = qr * Q;
    /* yaw (rotation about Z) */
    float yaw = atan2f(2.0f * (r * k + i * j), 1.0f - 2.0f * (j * j + k * k));
    *yaw_out = yaw * 57.2957795f;
    return true;
}

esp_err_t imu_bno086_init(i2c_master_bus_handle_t bus)
{
    if (bus == NULL) return ESP_ERR_INVALID_ARG;

    if (i2c_master_probe(bus, BNO_ADDR, 200) != ESP_OK) {
        ESP_LOGW(TAG, "BNO086 not found at 0x%02X -> IMU disabled (UWB/touch unaffected)", BNO_ADDR);
        return ESP_ERR_NOT_FOUND;
    }

    i2c_device_config_t dc = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = BNO_ADDR,
        .scl_speed_hz    = BNO_I2C_HZ,
    };
    esp_err_t e = i2c_master_bus_add_device(bus, &dc, &s_dev);
    if (e != ESP_OK) {
        ESP_LOGW(TAG, "add_device failed (%s) -> IMU disabled", esp_err_to_name(e));
        return e;
    }

    ESP_LOGI(TAG, "BNO086 @0x%02X found; enabling report 0x%02X @ %d Hz",
             BNO_ADDR, IMU_FEATURE_ID, 1000000 / REPORT_INTERVAL_US);

    /* Drain boot advertisement / init packets. */
    uint8_t buf[64]; uint8_t ch; int plen;
    for (int n = 0; n < 10; n++) { read_packet(buf, sizeof(buf), &plen, &ch); vTaskDelay(pdMS_TO_TICKS(3)); }

    enable_feature(IMU_FEATURE_ID, REPORT_INTERVAL_US);
    return ESP_OK;
}

bool imu_bno086_ok(void) { return s_ok; }

bool imu_bno086_get_heading(float *yaw_deg)
{
    if (!s_ok) return false;
    if (yaw_deg) *yaw_deg = s_yaw;
    return true;
}

static void imu_task(void *arg)
{
    (void)arg;
    uint8_t buf[64]; uint8_t ch; int plen;
    uint32_t got = 0, ticks = 0;
    while (1) {
        if (read_packet(buf, sizeof(buf), &plen, &ch) && plen > 0 && ch == CH_REPORTS) {
            float yaw;
            if (parse_rotation_vector(buf, plen, &yaw)) {
                s_yaw = yaw;
                s_ok = true;
                got++;
            }
        }
        /* Re-arm the feature if nothing arrived in the first ~2 s (enable may have
         * landed during the boot advertisement). */
        if (!s_ok && (++ticks % 100) == 0) {
            enable_feature(IMU_FEATURE_ID, REPORT_INTERVAL_US);
        }
        if ((ticks % 250) == 0) {
            ESP_LOGI(TAG, "heading=%.1f deg  ok=%d  samples=%u", s_yaw, (int)s_ok, (unsigned)got);
        }
        ticks++;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void imu_bno086_start_task(void)
{
    if (s_dev == NULL) return;   /* init failed -> no task */
    xTaskCreate(imu_task, "imu_rx", 4096, NULL, 4, NULL);
}
