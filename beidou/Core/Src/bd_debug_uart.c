#include "bd_debug_uart.h"

#include "bd_uart_ring.h"
#include "main.h"
#include "usart.h"
#include <stdio.h>

extern UART_HandleTypeDef huart2;

#if BD_DEBUG_UART_RAW_FORWARD
#define FWD_RING_CAP 2048u
static bd_ring_t s_fwd_ring;
static uint8_t s_fwd_buf[FWD_RING_CAP];
#endif

void bd_debug_uart_forward_init(void) {
#if BD_DEBUG_UART_RAW_FORWARD
  bd_ring_init(&s_fwd_ring, s_fwd_buf, FWD_RING_CAP);
#endif
}

void bd_debug_uart_rx_byte_for_forward(uint8_t b) {
#if BD_DEBUG_UART_RAW_FORWARD
  (void)bd_ring_push(&s_fwd_ring, b);
#endif
}

void bd_debug_uart_forward_flush(void) {
#if BD_DEBUG_UART_RAW_FORWARD
  uint8_t chunk[128];
  for (unsigned pass = 0u; pass < 8u; pass++) {
    size_t n = 0;
    while (n < sizeof(chunk) && bd_ring_pop(&s_fwd_ring, &chunk[n])) {
      n++;
    }
    if (n == 0u) {
      break;
    }
    (void)HAL_UART_Transmit(&huart2, chunk, (uint16_t)n, 250);
  }
#endif
}

void bd_debug_uart_print_pos(const bd_nmea_position_t *pos) {
  if (!pos) {
    return;
  }
  char buf[160];
  int n = snprintf(buf, sizeof(buf),
                   "[BD] lat_deg=%.8f lon_deg=%.8f fix=%d sats=%u\r\n",
                   pos->ll.lat_deg,
                   pos->ll.lon_deg,
                   (int)pos->fix,
                   (unsigned)pos->sats);
  if (n <= 0 || n >= (int)sizeof(buf)) {
    return;
  }
  (void)HAL_UART_Transmit(&huart2, (uint8_t *)buf, (uint16_t)n, 300);
}

void bd_debug_uart_boot_line(void) {
#if BD_DEBUG_UART_RAW_FORWARD || BD_DEBUG_UART_NMEA_LOG
  static const char msg[] = "[BD] USART2 OK; raw USART1->PC if RAW_FORWARD\r\n";
  (void)HAL_UART_Transmit(&huart2, (uint8_t *)msg, (uint16_t)(sizeof(msg) - 1u), 150);
#endif
}

void bd_debug_uart_log_nmea_line(const char *line, size_t len) {
#if BD_DEBUG_UART_NMEA_LOG && !BD_DEBUG_UART_RAW_FORWARD
  if (!line || len == 0u) {
    return;
  }
  if (len > 220u) {
    len = 220u;
  }
  (void)HAL_UART_Transmit(&huart2, (uint8_t *)line, (uint16_t)len, 200);
#endif
}
