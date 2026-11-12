#include "logger.h"
#include "usart.h"
#include <string.h>

// Should not be called in ISR
void send_log(char *msg) {

  HAL_UART_Transmit(&hlpuart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}