#pragma once

#include <stdint.h>
#include <stddef.h>
#include "bd_nmea.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 1=把 USART1 收到的每个字节原样转发到 USART2(PA2)，PC 串口助手可看原始协议（二进制/文本均可）。
 * 打开时自动关闭「仅转发校验通过的整行 NMEA」，避免重复。
 */
#ifndef BD_DEBUG_UART_RAW_FORWARD
#define BD_DEBUG_UART_RAW_FORWARD 1
#endif

#if BD_DEBUG_UART_RAW_FORWARD
#undef BD_DEBUG_UART_NMEA_LOG
#define BD_DEBUG_UART_NMEA_LOG 0
#else
#ifndef BD_DEBUG_UART_NMEA_LOG
#define BD_DEBUG_UART_NMEA_LOG 1
#endif
#endif

void bd_debug_uart_forward_init(void);
void bd_debug_uart_forward_flush(void);
void bd_debug_uart_rx_byte_for_forward(uint8_t b);

/**
 * 将当前解析结果经 USART2（PA2 TX）以 ASCII 打出，便于在 PC 串口助手上核对
 * MCU 是否收到并算出了经纬度（十进制度）。
 *
 * 注意：与 TTS 模块共用 USART2 时，调试数据会发到 TTS 的 RX，可能误触发播报。
 *       测本功能时建议暂时断开 TTS 的 RX，或仅接 PA2→PC、不接 TTS。
 */
void bd_debug_uart_print_pos(const bd_nmea_position_t *pos);

/**
 * 输出一条已通过 NMEA 校验和的完整语句（仅当 BD_DEBUG_UART_NMEA_LOG=1 且未开 RAW 转发）。
 */
void bd_debug_uart_log_nmea_line(const char *line, size_t len);

/** 上电发一行 ASCII，确认 PA2→USB-TTL→PC 链路 */
void bd_debug_uart_boot_line(void);

#ifdef __cplusplus
}
#endif
