#pragma once

#include <stdint.h>

/* USART1 北斗模块 RX：在 stm32f1xx_it.c 的 IRQ 里每收到 1 字节 +1 */
extern volatile uint32_t bd_uart1_rx_bytes; /* 定义在 stm32f1xx_it.c */
