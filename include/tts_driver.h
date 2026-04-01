#ifndef __TTS_DRIVER_H
#define __TTS_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 TTS 模块（VTX316）
 * @note 需在 CubeMX 里开启 USART2，115200 8N1
 */
void tts_init(void);

/**
 * @brief 发送文本让 TTS 播报
 * @param text 文本内容（UTF-8 编码）
 * @return true 发送成功（不代表播报成功）
 */
bool tts_speak(const char *text);

/**
 * @brief 停止当前播报
 */
void tts_stop(void);

/**
 * @brief 设置音量 (0-10)
 */
void tts_set_volume(uint8_t level);

/**
 * @brief 设置语速 (0-10)
 */
void tts_set_speed(uint8_t level);

/**
 * @brief 设置发音人 (1-7)
 */
void tts_set_voice(uint8_t id);

#ifdef __cplusplus
}
#endif

#endif /* __TTS_DRIVER_H */