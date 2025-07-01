#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void ntp_initialize(void);
void ntp_wait_for_sync(void);

#ifdef __cplusplus
}
#endif
