#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void ntp_initialize(void);
void ntp_wait_for_sync(void);
