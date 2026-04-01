#include "ui_display.h"
#include "lcd_hal.h"
#include "main.h"
#include "bd_nmea.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* 8x16 点阵时建议 sizey=16，否则步进为 0 会导致字符重叠 */
#define UI_FONT_H 16

/*
 * 坏屏适配：只在 [UI_GOOD_Y0, UI_GOOD_Y0 + UI_GOOD_H) 内作画（相对坐标用 UI_VY）。
 * 默认整屏；若只有半屏可用再改 GOOD_Y0 / GOOD_H。
 */
#define UI_GOOD_Y0 0u
#define UI_GOOD_H  80u

#if (UI_GOOD_Y0 + UI_GOOD_H) > LCD_H
#error UI_GOOD_Y0 + UI_GOOD_H must be <= LCD_H
#endif

#define UI_VY(y_rel) ((uint16_t)(UI_GOOD_Y0 + (uint16_t)(y_rel)))

// 颜色定义
#define UI_COLOR_BG       BLACK
#define UI_COLOR_TEXT     WHITE
#define UI_COLOR_ACCENT   GREEN
#define UI_COLOR_TITLE    CYAN

/* 相对可用区顶部的行位置（8x16 两行约占 0~34） */
#define TITLE_REL   4u
#define STATUS_REL  22u

static void ui_fill_full(uint16_t color) {
    LCD_Fill(0, 0, LCD_W, LCD_H, color);
}

static void ui_fill_workspace(uint16_t color) {
    LCD_Fill(0, UI_GOOD_Y0, LCD_W, UI_GOOD_Y0 + UI_GOOD_H, color);
}

void ui_display_init(void) {
    LCD_Init();
    ui_fill_full(UI_COLOR_BG);
    ui_fill_workspace(UI_COLOR_BG);
    ui_show_status("Beidou System");
}

void ui_show_landmark(const char *title) {
    if (!title) return;
    ui_fill_full(UI_COLOR_BG);
    ui_fill_workspace(UI_COLOR_BG);
    LCD_ShowString(4, UI_VY(TITLE_REL), (const uint8_t*)title, UI_COLOR_TITLE, UI_COLOR_BG, UI_FONT_H);
    LCD_ShowString(4, UI_VY(STATUS_REL), (const uint8_t*)"Playing...", UI_COLOR_ACCENT, UI_COLOR_BG, UI_FONT_H);
}

void ui_clear(void) {
    ui_fill_full(UI_COLOR_BG);
    ui_fill_workspace(UI_COLOR_BG);
    LCD_ShowString(4, UI_VY(STATUS_REL), (const uint8_t*)"Waiting...", UI_COLOR_TEXT, UI_COLOR_BG, UI_FONT_H);
}

void ui_show_position(double lat, double lon, uint8_t sats) {
    char buf[40];
    ui_fill_full(UI_COLOR_BG);
    ui_fill_workspace(UI_COLOR_BG);
    snprintf(buf, sizeof(buf), "Sats:%u", (unsigned)sats);
    LCD_ShowString(4, UI_VY(4), (const uint8_t*)buf, UI_COLOR_TEXT, UI_COLOR_BG, UI_FONT_H);
    snprintf(buf, sizeof(buf), "N:%.5f", lat);
    LCD_ShowString(4, UI_VY(22), (const uint8_t*)buf, UI_COLOR_TEXT, UI_COLOR_BG, UI_FONT_H);
    snprintf(buf, sizeof(buf), "E:%.5f", lon);
    LCD_ShowString(4, UI_VY(40), (const uint8_t*)buf, UI_COLOR_TEXT, UI_COLOR_BG, UI_FONT_H);
}

static char s_bd_line_last[5][32];
static uint8_t s_bd_line_inited;

void ui_bd_live_reset(void) {
    memset(s_bd_line_last, 0, sizeof(s_bd_line_last));
    s_bd_line_inited = 0u; /* 下一帧 ui_show_bd_live 会整区清屏并重画各行 */
}

void ui_show_bd_live(const bd_nmea_position_t *pos, uint32_t rx_total, uint32_t rx_per_sec) {
    char line[5][32];
    uint16_t y;

    snprintf(line[0], sizeof(line[0]), "RX:%lu %lu/s TX:--",
             (unsigned long)rx_total, (unsigned long)rx_per_sec);

    if (!pos) {
        snprintf(line[1], sizeof(line[1]), "POS:NULL");
        snprintf(line[2], sizeof(line[2]), "HDOP:-- A:--");
        snprintf(line[3], sizeof(line[3]), "N:--------");
        snprintf(line[4], sizeof(line[4]), "E:--------");
    } else {
        if (pos->fix == BD_NMEA_FIX_VALID) {
            snprintf(line[1], sizeof(line[1]), "FIX:OK S:%u", (unsigned)pos->sats);
        } else {
            snprintf(line[1], sizeof(line[1]), "FIX:-- S:%u", (unsigned)pos->sats);
        }
        if (pos->hdop >= 0.0f) {
            snprintf(line[2], sizeof(line[2]), "HDOP:%.1f A:%.0fm",
                     (double)pos->hdop, (double)pos->altitude_m);
        } else {
            snprintf(line[2], sizeof(line[2]), "HDOP:-- A:%.0fm", (double)pos->altitude_m);
        }
        if (pos->fix == BD_NMEA_FIX_VALID) {
            snprintf(line[3], sizeof(line[3]), "N:%.5f", pos->ll.lat_deg);
            snprintf(line[4], sizeof(line[4]), "E:%.5f", pos->ll.lon_deg);
        } else {
            snprintf(line[3], sizeof(line[3]), "N:--------");
            snprintf(line[4], sizeof(line[4]), "E:--------");
        }
    }

    if (!s_bd_line_inited) {
        ui_fill_full(UI_COLOR_BG);
        ui_fill_workspace(UI_COLOR_BG);
        s_bd_line_inited = 1;
    }

    for (unsigned i = 0; i < 5u; i++) {
        if (strcmp(line[i], s_bd_line_last[i]) == 0) {
            continue;
        }
        y = UI_VY((uint16_t)(i * 16u));
        LCD_Fill(0, y, LCD_W, (uint16_t)(y + UI_FONT_H), UI_COLOR_BG);
        {
            uint16_t fc = UI_COLOR_TEXT;
            if (i == 0u) {
                fc = UI_COLOR_ACCENT;
            } else if (i >= 3u && (!pos || pos->fix != BD_NMEA_FIX_VALID)) {
                fc = YELLOW;
            }
            LCD_ShowString(2, y, (const uint8_t*)line[i], fc, UI_COLOR_BG, UI_FONT_H);
        }
        strncpy(s_bd_line_last[i], line[i], sizeof(s_bd_line_last[i]) - 1u);
        s_bd_line_last[i][sizeof(s_bd_line_last[i]) - 1u] = '\0';
    }
}

void ui_show_status(const char *status) {
    if (!status) return;
    ui_fill_full(UI_COLOR_BG);
    ui_fill_workspace(UI_COLOR_BG);
    LCD_ShowString(4, UI_VY(STATUS_REL), (const uint8_t*)status, UI_COLOR_TEXT, UI_COLOR_BG, UI_FONT_H);
}

void ui_lcd_quick_pattern(void) {
    ui_fill_full(BLACK);
    LCD_Fill(0, UI_GOOD_Y0, LCD_W, UI_GOOD_Y0 + UI_GOOD_H / 2, RED);
    LCD_Fill(0, UI_GOOD_Y0 + UI_GOOD_H / 2, LCD_W, UI_GOOD_Y0 + UI_GOOD_H, GREEN);
    LCD_ShowString(2, UI_VY(2), (const uint8_t*)"UP RED", WHITE, RED, UI_FONT_H);
    LCD_ShowString(2, UI_VY(22), (const uint8_t*)"DN GRN", BLACK, GREEN, UI_FONT_H);
}

void ui_lcd_self_test_sequence(void) {
    ui_lcd_quick_pattern();
    HAL_Delay(1500);

    ui_fill_full(BLUE);
    ui_fill_workspace(BLUE);
    LCD_ShowString(2, UI_VY(12), (const uint8_t*)"FULL BLUE", WHITE, BLUE, UI_FONT_H);
    HAL_Delay(1000);

    ui_fill_full(YELLOW);
    ui_fill_workspace(YELLOW);
    LCD_ShowString(2, UI_VY(12), (const uint8_t*)"FULL YEL", BLACK, YELLOW, UI_FONT_H);
    HAL_Delay(1000);

    ui_fill_full(BLACK);
    ui_fill_workspace(BLACK);
    LCD_ShowString(2, UI_VY(0), (const uint8_t*)"ROW1----", WHITE, BLACK, UI_FONT_H);
    LCD_ShowString(2, UI_VY(18), (const uint8_t*)"ROW2----", WHITE, BLACK, UI_FONT_H);
    HAL_Delay(2000);

    ui_fill_full(BLACK);
    ui_fill_workspace(BLACK);
    LCD_ShowString(2, UI_VY(12), (const uint8_t*)"LCD OK", GREEN, BLACK, UI_FONT_H);
}

void ui_lcd_region_map_test(void) {
    const uint16_t fh = UI_FONT_H;
    const uint16_t midy = LCD_H / 2;
    const uint16_t midx = LCD_W / 2;

    /* 1) 整屏三色：判断大面积是否同显 */
    LCD_Fill(0, 0, LCD_W, LCD_H, RED);
    LCD_ShowString(4, 28, (const uint8_t*)"FULL RED", WHITE, RED, fh);
    HAL_Delay(2200);

    LCD_Fill(0, 0, LCD_W, LCD_H, GREEN);
    LCD_ShowString(4, 28, (const uint8_t*)"FULL GRN", BLACK, GREEN, fh);
    HAL_Delay(2200);

    LCD_Fill(0, 0, LCD_W, LCD_H, BLUE);
    LCD_ShowString(4, 28, (const uint8_t*)"FULL BLUE", WHITE, BLUE, fh);
    HAL_Delay(2200);

    /* 2) 上下半：红上绿下，文字标 TOP / BOT */
    LCD_Fill(0, 0, LCD_W, midy, RED);
    LCD_Fill(0, midy, LCD_W, LCD_H, GREEN);
    LCD_ShowString(4, 8, (const uint8_t*)"TOP", WHITE, RED, fh);
    LCD_ShowString(4, (uint16_t)(midy + 8), (const uint8_t*)"BOT", BLACK, GREEN, fh);
    HAL_Delay(2800);

    /* 3) 左右半：青左黄右，标 L / R */
    LCD_Fill(0, 0, midx, LCD_H, CYAN);
    LCD_Fill(midx, 0, LCD_W, LCD_H, YELLOW);
    LCD_ShowString(8, 28, (const uint8_t*)"L", BLACK, CYAN, fh);
    LCD_ShowString((uint16_t)(midx + 8), 28, (const uint8_t*)"R", BLACK, YELLOW, fh);
    HAL_Delay(2800);

    /* 4) 四象限：1=左上 2=右上 3=左下 4=右下 */
    LCD_Fill(0, 0, midx, midy, RED);
    LCD_Fill(midx, 0, LCD_W, midy, GREEN);
    LCD_Fill(0, midy, midx, LCD_H, CYAN);
    LCD_Fill(midx, midy, LCD_W, LCD_H, BLUE);
    LCD_ShowString(8, 10, (const uint8_t*)"1", WHITE, RED, fh);
    LCD_ShowString((uint16_t)(midx + 8), 10, (const uint8_t*)"2", BLACK, GREEN, fh);
    LCD_ShowString(8, (uint16_t)(midy + 10), (const uint8_t*)"3", BLACK, CYAN, fh);
    LCD_ShowString((uint16_t)(midx + 8), (uint16_t)(midy + 10), (const uint8_t*)"4", WHITE, BLUE, fh);
    HAL_Delay(3500);

    /* 5) 结束：黑底提示 */
    LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
    LCD_ShowString(2, 12, (const uint8_t*)"MAP END", GREEN, BLACK, fh);
    LCD_ShowString(2, 36, (const uint8_t*)"REPEAT..", CYAN, BLACK, fh);
    HAL_Delay(1200);
}

void ui_lcd_madctl_probe(void) {
    const uint16_t fh = UI_FONT_H;
    static const uint8_t mad[] = { 0x00u, 0x60u, 0xC0u, 0xC8u };
    static const char *const lab[] = { "MAD 00", "MAD 60", "MAD C0", "MAD C8" };
    static const uint16_t col[] = { RED, GREEN, BLUE, YELLOW };

    for (unsigned i = 0; i < 4u; i++) {
        LCD_Set_MADCTL(mad[i]);
        LCD_Fill(0, 0, LCD_W, LCD_H, col[i]);
        uint16_t fc = (col[i] == GREEN || col[i] == YELLOW) ? BLACK : WHITE;
        LCD_ShowString(4, 28, (const uint8_t *)lab[i], fc, col[i], fh);
        HAL_Delay(2800);
    }
    LCD_Set_MADCTL(LCD_MADCTL);
    LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
    LCD_ShowString(2, 24, (const uint8_t*)"PROBE END", GREEN, BLACK, fh);
    HAL_Delay(1200);
}

void ui_lcd_text_demo(void) {
    const uint16_t fh = UI_FONT_H;
    static uint32_t s_round;
    char buf[28];

    s_round++;

    LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
    LCD_ShowString(4, 2, (const uint8_t*)"Beidou LCD Test", CYAN, BLACK, fh);
    LCD_ShowString(4, 20, (const uint8_t*)"ABCDEF 0123456789", WHITE, BLACK, fh);
    snprintf(buf, sizeof(buf), "Tick:%lu", (unsigned long)HAL_GetTick());
    LCD_ShowString(4, 38, (const uint8_t*)buf, GREEN, BLACK, fh);
    snprintf(buf, sizeof(buf), "Round %lu", (unsigned long)s_round);
    LCD_ShowString(4, 56, (const uint8_t*)buf, YELLOW, BLACK, fh);
    HAL_Delay(2800);

    LCD_Fill(0, 0, LCD_W, LCD_H, BLUE);
    LCD_ShowString(4, 8, (const uint8_t*)"White on Blue", WHITE, BLUE, fh);
    LCD_ShowString(4, 28, (const uint8_t*)"Yellow line 2", YELLOW, BLUE, fh);
    LCD_ShowString(4, 48, (const uint8_t*)"Text demo OK", CYAN, BLUE, fh);
    HAL_Delay(2800);
}
