#ifndef __LCD_HAL_H
#define __LCD_HAL_H

#include "main.h"
#include <stdint.h>

// LCD Pin definitions (PB3-8)
#define LCD_SCL_PIN   GPIO_PIN_3
#define LCD_SDA_PIN   GPIO_PIN_5
#define LCD_RES_PIN   GPIO_PIN_6
#define LCD_DC_PIN    GPIO_PIN_4
#define LCD_CS_PIN    GPIO_PIN_7
#define LCD_BLK_PIN   GPIO_PIN_8
#define LCD_PORT      GPIOB

// Color definitions
#define WHITE   0xFFFF
#define BLACK   0x0000  
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define YELLOW  0xFFE0

/* 逻辑分辨率（与当前绘图坐标一致）；常见 0.96" ST7735S 横屏为 160x80 */
#define LCD_W 160
#define LCD_H 80

/*
 * 与 stm32f1_Lib 里两套「STM32F103 TFT开发板综合测试」一致（同引脚 PB3~8）：
 *
 * - 京东方玻璃：HARDWARE/LCD/lcd_init.c — 横屏 USE_HORIZONTAL=2 时 MADCTL=0x78，
 *   列地址不加偏移，行地址 +24（与竖屏 0/1 时「列+24、行不变」相反）。
 * - 瀚彩玻璃：  按卖家例程调整列/行偏移；另含 0xFC、0x21 等与京东方不同的 init。
 *
 * 注意：卖家例程里 0xC8 对应竖屏 (USE_HORIZONTAL=1)，0x78/0xA8 对应横屏 (2/3)。
 * 本工程 LCD_W=160、LCD_H=80 为横屏逻辑坐标，默认按京东方横屏对齐。
 * 若你手上是瀚彩玻璃，请对照其 lcd_init.c 里 Address_Set 修改 LCD_X_OFFSET / LCD_Y_OFFSET。
 */
#ifndef LCD_X_OFFSET
#define LCD_X_OFFSET 0
#endif
#ifndef LCD_Y_OFFSET
#define LCD_Y_OFFSET 24
#endif
#ifndef LCD_MADCTL
#define LCD_MADCTL 0x78u
#endif

// Functions
void LCD_Init(void);
/** 运行时改扫描方向（0x36 MADCTL），用于横竖方向试探；与 LCD_MADCTL 初始值独立 */
void LCD_Set_MADCTL(uint8_t madctl);
void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color);
void LCD_ShowString(uint16_t x, uint16_t y, const uint8_t *p, uint16_t fc, uint16_t bc, uint8_t sizey);
void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);

#endif