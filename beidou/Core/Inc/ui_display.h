#ifndef __UI_DISPLAY_H
#define __UI_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#include "bd_nmea.h"

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
 * @brief 精简北斗界面
 * - nav_enabled=0：显示解析/解包是否有成功（parse_ok_seen），便于等信号前确认链路通
 * - nav_enabled=1：只关注 FIX/经纬度（进入地标才播报）
 */
void ui_show_bd_live(const bd_nmea_position_t *pos, bool nav_enabled, bool parse_ok_seen);

/** 切换界面后若需重新进入北斗调试全屏，可先调用以强制下一帧重绘全部行 */
void ui_bd_live_reset(void);

/**
 * @brief 显示状态信息（如"搜索卫星中..."）
 * @param status 状态文本
 */
void ui_show_status(const char *status);

/**
 * @brief 屏幕自检：上下分色、全屏换色、多行文字（用于判断是否花屏/断线）
 * @note 约 4~5 秒，阻塞执行；字体步进使用 sizey=16
 */
void ui_lcd_self_test_sequence(void);

/**
 * @brief 简单色条：上红下绿 + 四行文字，便于快速看半屏是否异常
 */
void ui_lcd_quick_pattern(void);

/**
 * @brief 分区图：整屏纯色 → 上下半 → 左右半 → 四象限（用于判断哪一块区域能显示）
 * @note 使用整屏坐标，不受 UI_GOOD_Y0/H 限制；阻塞约 15s 一轮
 */
void ui_lcd_region_map_test(void);

/**
 * @brief 横竖方向：依次切换 MADCTL 0x00/0x60/0xC0/0xC8，整屏填色+标号，用于选对与 FPC 一致的方向
 * @note 看中某一档后，把 lcd_hal.h 里 LCD_MADCTL 改成对应十六进制并重新编译
 */
void ui_lcd_madctl_probe(void);

/**
 * @brief 文字测试：多行 ASCII、双色底、Tick/轮次（整屏，约数秒一轮）
 */
void ui_lcd_text_demo(void);

#ifdef __cplusplus
}
#endif

#endif
