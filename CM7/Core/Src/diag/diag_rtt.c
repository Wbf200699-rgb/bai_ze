#include "diag_rtt.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "SEGGER_RTT.h"
#include "stm32h7xx.h"

void diag_rtt_init(void)
{
    SEGGER_RTT_Init();
    SEGGER_RTT_SetFlagsUpBuffer(0, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
}

void diag_rtt_printf(const char *fmt, ...)
{
    char line[256];
    va_list args;
    int length;
    uint32_t primask;

    va_start(args, fmt);
    length = vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    if (length <= 0) {
        return;
    }
    if ((unsigned)length >= sizeof(line)) {
        length = (int)sizeof(line) - 1;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    (void)SEGGER_RTT_WriteSkipNoLock(0, line, (unsigned)length);
    if (primask == 0U) {
        __enable_irq();
    }
}

void diag_rtt_ec20_rx_isr(const uint8_t *data, uint16_t len)
{
    static const char prefix[] = "[EC20 RX] ";
    static const char suffix[] = "\r\n";
    uint16_t shown = len;

    if (shown > 192U) {
        shown = 192U;
    }
    (void)SEGGER_RTT_WriteSkipNoLock(0, prefix, sizeof(prefix) - 1U);
    (void)SEGGER_RTT_WriteSkipNoLock(0, data, shown);
    if (shown < len) {
        static const char clipped[] = " ...[clipped]";
        (void)SEGGER_RTT_WriteSkipNoLock(0, clipped, sizeof(clipped) - 1U);
    }
    (void)SEGGER_RTT_WriteSkipNoLock(0, suffix, sizeof(suffix) - 1U);
}
