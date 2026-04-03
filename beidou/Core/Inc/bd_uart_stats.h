#pragma once

#include <stdint.h>

/* 北斗收包字节数：USART1 模式在 USART1_IRQHandler；USART2 模式在 USART2_IRQHandler（变量名未改） */
extern volatile uint32_t bd_uart1_rx_bytes; /* 定义在 stm32f1xx_it.c */
/* 进对应 USART 全局中断次数（用于确认是否进过中断） */
extern volatile uint32_t bd_uart1_irq_entries;
/* USART1 硬件错误：在 IRQ 内根据 SR 统计（与接线/波特率/过载相关） */
extern volatile uint32_t bd_uart1_ore; /* Overrun */
extern volatile uint32_t bd_uart1_fe;  /* Framing */
extern volatile uint32_t bd_uart1_ne;  /* Noise */

/** 可选：主循环读寄存器填好后用于自定义调试显示（默认 UI 已不依赖此结构） */
typedef struct {
    uint32_t rx_bytes;
    uint32_t rx_per_sec;
    uint32_t irq_entries;
    uint32_t ore;
    uint32_t fe;
    uint32_t ne;
    uint8_t usart_ue;   /* USART1->CR1 UE */
    uint8_t rcc_clk_on; /* RCC_APB2ENR USART1EN */
    uint32_t nmea_lines;
    uint32_t checksum_ok;
    uint32_t checksum_bad;
    uint32_t rmc_nt;
    uint32_t gga_nt;
    uint32_t other_nt;
    uint32_t parse_ok;
    uint32_t parse_skip;
} bd_uart_diag_t;
