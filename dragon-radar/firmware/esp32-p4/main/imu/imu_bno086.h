#pragma once
/*
 * BNO086 (CEVA/Hillcrest SH-2 over SHTP/I2C) minimal driver for dragon-radar.
 * Shares the BSP I2C bus (SCL=GPIO8 / SDA=GPIO7, addr 0x4B) with the GT911 touch.
 * Provides the device heading (yaw, deg) so the UWB azimuth can be rotated into a
 * world frame ("dragon radar" blip stays put when the device is turned).
 *
 * Graceful: if the BNO086 is absent on the bus, init returns ESP_ERR_NOT_FOUND and
 * the rest of the firmware (UWB + touch) runs untouched.
 */
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

/* Probe 0x4B, add it to the shared bus, enable the rotation-vector report. */
esp_err_t imu_bno086_init(i2c_master_bus_handle_t bus);

/* Spawn the polling task (no-op if init failed). */
void imu_bno086_start_task(void);

/* true once at least one valid heading sample has been parsed. */
bool imu_bno086_ok(void);

/* Latest yaw/heading in degrees, range [-180, 180). Returns false if not ready. */
bool imu_bno086_get_heading(float *yaw_deg);
