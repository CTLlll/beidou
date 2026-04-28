#include "tts_driver.h"
#include "main.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

// VTX316 播放帧格式: FD 00 (len+2) 01 05 [GBK text]
// 音量设置帧: FD 00 06 01 01 5B 76 [30+level] 5D
// 停止帧: FD 00 01 03
// 暂停帧: FD 00 01 03
// 恢复帧: FD 00 01 04
// 发音人帧: FD 00 (len+2) 01 05 [m5X]

// VTX316 uses GBK encoding for Chinese text
// We'll need a simple GBK conversion - for now send as ASCII/UTF-8
// In production, you'd add GBK conversion table or library

extern UART_HandleTypeDef huart1;

/* Text coding selector for synthesis command.
   0x00 works on many modules for GBK/GB2312 byte streams.
   If needed, switch to 0x05 to test vendor-specific mode. */
#ifndef TTS_TEXT_CODING
#define TTS_TEXT_CODING 0x00u
#endif

void tts_init(void) {
    // 可以在这里做模块初始化，例如设置默认音量
    // TTS 模块上电后默认可以工作
}

bool tts_speak_bytes(const uint8_t *data, uint16_t len) {
    if (!data || len == 0u) {
        return false;
    }

    size_t frame_len = (size_t)len + 2u;  // payload + cmd/type bytes
    uint8_t frame[5] = {
        0xFD,
        0x00,
        (uint8_t)(frame_len & 0xFF),
        0x01,
        (uint8_t)TTS_TEXT_CODING
    };

    if (HAL_UART_Transmit(&huart1, frame, sizeof(frame), 120) != HAL_OK) {
        return false;
    }
    if (HAL_UART_Transmit(&huart1, (uint8_t *)data, len, 800) != HAL_OK) {
        return false;
    }
    return true;
}

bool tts_speak(const char *text) {
    if (!text || *text == '\0') {
        return false;
    }

    /* text is already raw bytes (ASCII or GBK byte sequence like \xD5\xE2...).
       Use exact byte length to keep frame length fully consistent. */
    size_t text_len = strlen(text);
    if (text_len > 0xFFFFu) {
        return false;
    }
    return tts_speak_bytes((const uint8_t *)text, (uint16_t)text_len);
}

void tts_stop(void) {
    uint8_t cmd[] = {0xFD, 0x00, 0x01, 0x03};
    HAL_UART_Transmit(&huart1, cmd, sizeof(cmd), 100);
}

void tts_set_volume(uint8_t level) {
    if (level > 10) level = 10;

    uint8_t cmd[] = {
        0xFD, 0x00, 0x06,
        0x01, 0x01,
        0x5B, 0x76,
        (uint8_t)(0x30 + level),
        0x5D
    };
    HAL_UART_Transmit(&huart1, cmd, sizeof(cmd), 100);
}

void tts_set_speed(uint8_t level) {
    if (level > 10) level = 10;

    // 语速设置命令 (与音量格式类似，只是参数区不同)
    uint8_t cmd[] = {
        0xFD, 0x00, 0x06,
        0x01, 0x01,
        0x5B, 0x78,
        (uint8_t)(0x30 + level),
        0x5D
    };
    HAL_UART_Transmit(&huart1, cmd, sizeof(cmd), 100);
}

void tts_set_voice(uint8_t id) {
    if (id < 1) id = 1;
    if (id > 7) id = 7;

    // 构建选择发音人命令文本: [m5X]
    char cmd_str[8];
    int len = snprintf(cmd_str, sizeof(cmd_str), "[m5%d]", id);

    // 发送帧格式与 speak 类似
    uint8_t frame[6] = {
        0xFD, 0x00, (uint8_t)(len + 2), 0x01, 0x05
    };
    HAL_UART_Transmit(&huart1, frame, 5, 100);
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd_str, len, 100);
}
