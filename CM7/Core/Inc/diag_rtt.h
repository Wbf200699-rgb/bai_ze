#ifndef DIAG_RTT_H
#define DIAG_RTT_H

#include <stdint.h>

void diag_rtt_init(void);
void diag_rtt_printf(const char *fmt, ...);
void diag_rtt_ec20_rx_isr(const uint8_t *data, uint16_t len);

#endif
