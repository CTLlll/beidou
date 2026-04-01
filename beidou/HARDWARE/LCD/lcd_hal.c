#include "lcd_hal.h"

/* 与京东方例程 lcdfont.h 中 ascii_1608 一致 */
extern const uint8_t ascii_1608[95][16];

static void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color);

void LCD_WR_REG(uint8_t dat) {
    HAL_GPIO_WritePin(LCD_PORT, LCD_DC_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    
    for (int i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(LCD_PORT, LCD_SCL_PIN, GPIO_PIN_RESET);
        if (dat & 0x80) {
            HAL_GPIO_WritePin(LCD_PORT, LCD_SDA_PIN, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(LCD_PORT, LCD_SDA_PIN, GPIO_PIN_RESET);
        }
        HAL_GPIO_WritePin(LCD_PORT, LCD_SCL_PIN, GPIO_PIN_SET);
        dat <<= 1;
    }
    
    HAL_GPIO_WritePin(LCD_PORT, LCD_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_PORT, LCD_DC_PIN, GPIO_PIN_SET);
}

void LCD_WR_DATA8(uint8_t dat) {
    HAL_GPIO_WritePin(LCD_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    
    for (int i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(LCD_PORT, LCD_SCL_PIN, GPIO_PIN_RESET);
        if (dat & 0x80) {
            HAL_GPIO_WritePin(LCD_PORT, LCD_SDA_PIN, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(LCD_PORT, LCD_SDA_PIN, GPIO_PIN_RESET);
        }
        HAL_GPIO_WritePin(LCD_PORT, LCD_SCL_PIN, GPIO_PIN_SET);
        dat <<= 1;
    }
    
    HAL_GPIO_WritePin(LCD_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    const uint32_t ox = (uint32_t)LCD_X_OFFSET;
    const uint32_t oy = (uint32_t)LCD_Y_OFFSET;
    x1 = (uint16_t)(x1 + ox);
    x2 = (uint16_t)(x2 + ox);
    y1 = (uint16_t)(y1 + oy);
    y2 = (uint16_t)(y2 + oy);
    LCD_WR_REG(0x2A);
    LCD_WR_DATA8(x1 >> 8); LCD_WR_DATA8(x1 & 0xFF);
    LCD_WR_DATA8(x2 >> 8); LCD_WR_DATA8(x2 & 0xFF);
    LCD_WR_REG(0x2B);
    LCD_WR_DATA8(y1 >> 8); LCD_WR_DATA8(y1 & 0xFF);
    LCD_WR_DATA8(y2 >> 8); LCD_WR_DATA8(y2 & 0xFF);
    LCD_WR_REG(0x2C);
}

static void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color) {
    LCD_Address_Set(x, y, x, y);
    LCD_WR_DATA8((uint8_t)(color >> 8));
    LCD_WR_DATA8((uint8_t)(color & 0xFF));
}

void LCD_Set_MADCTL(uint8_t madctl) {
    LCD_WR_REG(0x36);
    LCD_WR_DATA8(madctl);
}

void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color) {
    uint16_t i, j;
    if (xend <= xsta || yend <= ysta) {
        return;
    }
    LCD_Address_Set(xsta, ysta, (uint16_t)(xend - 1), (uint16_t)(yend - 1));
    for (i = ysta; i < yend; i++) {
        for (j = xsta; j < xend; j++) {
            LCD_WR_DATA8((uint8_t)(color >> 8));
            LCD_WR_DATA8((uint8_t)(color & 0xFF));
        }
    }
}

void LCD_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = LCD_SCL_PIN | LCD_SDA_PIN | LCD_RES_PIN | LCD_DC_PIN | LCD_CS_PIN | LCD_BLK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    HAL_GPIO_WritePin(GPIOB, LCD_SCL_PIN | LCD_SDA_PIN | LCD_RES_PIN | LCD_DC_PIN | LCD_CS_PIN | LCD_BLK_PIN, GPIO_PIN_SET);
    
    HAL_GPIO_WritePin(LCD_PORT, LCD_RES_PIN, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(LCD_PORT, LCD_RES_PIN, GPIO_PIN_SET);
    HAL_Delay(100);
    
    HAL_GPIO_WritePin(LCD_PORT, LCD_BLK_PIN, GPIO_PIN_SET);
    HAL_Delay(100);

    LCD_WR_REG(0x11);
    HAL_Delay(120);
    LCD_WR_REG(0xB1);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);
    LCD_WR_REG(0xB2);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);
    LCD_WR_REG(0xB3);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);
    LCD_WR_REG(0xB4);
    LCD_WR_DATA8(0x03);
    LCD_WR_REG(0xC0);
    LCD_WR_DATA8(0x0E);
    LCD_WR_DATA8(0x0E);
    LCD_WR_DATA8(0x04);
    LCD_WR_REG(0xC1);
    LCD_WR_DATA8(0xC5);
    LCD_WR_REG(0xC2);
    LCD_WR_DATA8(0x0d);
    LCD_WR_DATA8(0x00);
    LCD_WR_REG(0xC3);
    LCD_WR_DATA8(0x8D);
    LCD_WR_DATA8(0x2A);
    LCD_WR_REG(0xC4);
    LCD_WR_DATA8(0x8D);
    LCD_WR_DATA8(0xEE);
    LCD_WR_REG(0xC5);
    LCD_WR_DATA8(0x06);
    LCD_WR_REG(0x36);
    LCD_WR_DATA8(LCD_MADCTL);
    LCD_WR_REG(0x3A);
    LCD_WR_DATA8(0x55);
    LCD_WR_REG(0xE0);
    LCD_WR_DATA8(0x0b);
    LCD_WR_DATA8(0x17);
    LCD_WR_DATA8(0x0a);
    LCD_WR_DATA8(0x0d);
    LCD_WR_DATA8(0x1a);
    LCD_WR_DATA8(0x19);
    LCD_WR_DATA8(0x16);
    LCD_WR_DATA8(0x1d);
    LCD_WR_DATA8(0x21);
    LCD_WR_DATA8(0x26);
    LCD_WR_DATA8(0x37);
    LCD_WR_DATA8(0x3c);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x10);
    LCD_WR_REG(0xE1);
    LCD_WR_DATA8(0x0c);
    LCD_WR_DATA8(0x19);
    LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x0d);
    LCD_WR_DATA8(0x1b);
    LCD_WR_DATA8(0x19);
    LCD_WR_DATA8(0x15);
    LCD_WR_DATA8(0x1d);
    LCD_WR_DATA8(0x21);
    LCD_WR_DATA8(0x26);
    LCD_WR_DATA8(0x39);
    LCD_WR_DATA8(0x3E);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x10);
    HAL_Delay(120);
    LCD_WR_REG(0x29);
}

void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode) {
    uint8_t temp, sizex;
    uint16_t i;
    uint16_t TypefaceNum;
    uint16_t x0 = x;
    uint8_t m = 0;

    if (sizey != 16) {
        return;
    }
    sizex = sizey / 2;
    TypefaceNum = (uint16_t)((sizex / 8 + ((sizex % 8) ? 1u : 0u)) * sizey);
    if (num < ' ' || num > '~') {
        return;
    }
    num = (uint8_t)(num - ' ');
    LCD_Address_Set(x, y, (uint16_t)(x + sizex - 1), (uint16_t)(y + sizey - 1));
    for (i = 0; i < TypefaceNum; i++) {
        temp = ascii_1608[num][i];
        for (uint8_t t = 0; t < 8; t++) {
            if (mode == 0) {
                if (temp & (uint8_t)(0x01u << t)) {
                    LCD_WR_DATA8((uint8_t)(fc >> 8));
                    LCD_WR_DATA8((uint8_t)(fc & 0xFF));
                } else {
                    LCD_WR_DATA8((uint8_t)(bc >> 8));
                    LCD_WR_DATA8((uint8_t)(bc & 0xFF));
                }
                m++;
                if ((m % sizex) == 0) {
                    m = 0;
                    break;
                }
            } else {
                if (temp & (uint8_t)(0x01u << t)) {
                    lcd_draw_point(x, y, fc);
                }
                x++;
                if ((uint16_t)(x - x0) == sizex) {
                    x = x0;
                    y++;
                    break;
                }
            }
        }
    }
}

void LCD_ShowString(uint16_t x, uint16_t y, const uint8_t *p, uint16_t fc, uint16_t bc, uint8_t sizey) {
    while (*p != '\0') {
        LCD_ShowChar(x, y, *p, fc, bc, sizey, 0);
        x += sizey / 2;
        p++;
    }
}
