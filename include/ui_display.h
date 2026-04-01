#ifndef __UI_DISPLAY_H
#define __UI_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 LCD 屏幕
 * @note 需要先调用 LCD_Init()（来自你的 lcd_init.c）
 */
void ui_display_init(void);

/**
 * @brief 显示地标信息（进入地标时调用）
 * @param title 标题文本
 */
void ui_show_landmark(const char *title);

/**
 * @brief 清屏/显示待机画面
 */
void ui_clear(void);

/**
 * @brief 显示调试信息（经纬度、卫星数等）
 * @param lat 纬度
 * @param lon 经纬度
 * @param sats 卫星数
 */
void ui_show_position(double lat, double lon, uint8_t sats);

/**
 * @brief 显示状态信息（如"搜索卫星中..."）
 * @param status 状态文本
 */
void ui_show_status(const char *status);

#ifdef __cplusplus
}
#endif

#endif /* __UI_DISPLAY_H */