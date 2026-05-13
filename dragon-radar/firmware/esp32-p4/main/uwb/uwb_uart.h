#pragma once

#include "ui/radar_view.h"

void uwb_uart_init(void);
bool uwb_uart_recv(uwb_measurement_t *out, uint32_t timeout_ms);
