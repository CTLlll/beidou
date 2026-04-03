#pragma once

/**
 * 北斗 NMEA 从哪路 UART 进 MCU（二选一）：
 * 0 = USART1，模块 TX → PA10
 * 1 = USART2，模块 TX → PA3（当前工程：北斗 USART2；TTS 走 USART1 PA9 TX）
 */
#ifndef BD_USE_USART2_FOR_GNSS
#define BD_USE_USART2_FOR_GNSS 1
#endif
