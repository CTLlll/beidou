#pragma once

#include <stdint.h>
#include <stddef.h>
#include "bd_nmea.h"
#include "bd_gnss_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 1=把「当前 GNSS 所用 UART」收到的字节原样转发到 USART2(PA2)，PC 接 PA2 可看（仅当 GNSS 走 USART1 时成立）。
 * BD_USE_USART2_FOR_GNSS=1 时强制关闭（避免与 PA3 收数混用同一转发口）。
 */
#ifndef BD_DEBUG_UART_RAW_FORWARD
#if BD_USE_USART2_FOR_GNSS
#define BD_DEBUG_UART_RAW_FORWARD 0
#else
#define BD_DEBUG_UART_RAW_FORWARD 1
#endif
#endif

#if BD_DEBUG_UART_RAW_FORWARD
#undef BD_DEBUG_UART_NMEA_LOG
#define BD_DEBUG_UART_NMEA_LOG 0
#else
#ifndef BD_DEBUG_UART_NMEA_LOG
#define BD_DEBUG_UART_NMEA_LOG 1
#endif
#endif

#if BD_USE_USART2_FOR_GNSS
#undef BD_DEBUG_UART_RAW_FORWARD
#define BD_DEBUG_UART_RAW_FORWARD 0
#ifndef BD_DEBUG_UART_NMEA_LOG
#define BD_DEBUG_UART_NMEA_LOG 0
#endif
#endif

void bd_debug_uart_forward_init(void);
void bd_debug_uart_forward_flush(void);
void bd_debug_uart_rx_byte_for_forward(uint8_t b);

/**
 * 将当前解析结果以 ASCII 经调试口打出（GNSS 走 USART2 时为 PA2 TX；GNSS 走 USART1 时为 PA2）。
 * BD_DEBUG_UART_POS=1 时使用；TTS 在 USART1(PA9) 时不占此调试口。
 */
void bd_debug_uart_print_pos(const bd_nmea_position_t *pos);

/**
 * 输出一条已通过 NMEA 校验和的完整语句（仅当 BD_DEBUG_UART_NMEA_LOG=1 且未开 RAW 转发）。
 */
void bd_debug_uart_log_nmea_line(const char *line, size_t len);

/** 上电发一行 ASCII：经 USART2 PA2（TTS 已迁 USART1，可与 GNSS 接收共存，仅多接一根 USB-TTL） */
void bd_debug_uart_boot_line(void);

#ifdef __cplusplus
}
#endif
