#include "ui_display.h"
#include "lcd.h"
#include "lcd_init.h"
#include <stdio.h>
#include <string.h>

// 颜色定义
#define UI_COLOR_BG       BLACK
#define UI_COLOR_TEXT     WHITE
#define UI_COLOR_ACCENT   GREEN
#define UI_COLOR_WARN     YELLOW
#define UI_COLOR_TITLE    CYAN

// 显示区域定义 (160x80 横屏)
#define TITLE_Y      10
#define STATUS_Y     35
#define POS_Y        55

void ui_display_init(void) {
    // 调用原有的 LCD 初始化
    LCD_Init();
    
    // 清屏
    LCD_Fill(0, 0, LCD_W, LCD_H, UI_COLOR_BG);
    
    // 显示欢迎信息
    ui_show_status("北斗定位系统");
}

void ui_show_landmark(const char *title) {
    if (!title) return;
    
    // 清屏
    LCD_Fill(0, 0, LCD_W, LCD_H, UI_COLOR_BG);
    
    // 显示标题
    LCD_ShowString(10, TITLE_Y, (u8*)title, UI_COLOR_TITLE, UI_COLOR_BG, 0);
    
    // 显示提示
    LCD_ShowString(10, STATUS_Y, (u8*)"播放语音中...", UI_COLOR_ACCENT, UI_COLOR_BG, 0);
}

void ui_clear(void) {
    LCD_Fill(0, 0, LCD_W, LCD_H, UI_COLOR_BG);
    LCD_ShowString(10, STATUS_Y, (u8*)"等待定位...", UI_COLOR_TEXT, UI_COLOR_BG, 0);
}

void ui_show_position(double lat, double lon, uint8_t sats) {
    char buf[32];
    
    // 第一行：卫星数
    snprintf(buf, sizeof(buf), "Sats:%d", sats);
    LCD_ShowString(10, 10, (u8*)buf, UI_COLOR_TEXT, UI_COLOR_BG, 0);
    
    // 第二行：纬度
    snprintf(buf, sizeof(buf), "N:%.5f", lat);
    LCD_ShowString(10, 25, (u8*)buf, UI_COLOR_TEXT, UI_COLOR_BG, 0);
    
    // 第三行：经度
    snprintf(buf, sizeof(buf), "E:%.5f", lon);
    LCD_ShowString(10, 40, (u8*)buf, UI_COLOR_TEXT, UI_COLOR_BG, 0);
}

void ui_show_status(const char *status) {
    if (!status) return;
    
    LCD_Fill(0, 0, LCD_W, LCD_H, UI_COLOR_BG);
    LCD_ShowString(10, STATUS_Y, (u8*)status, UI_COLOR_TEXT, UI_COLOR_BG, 0);
}