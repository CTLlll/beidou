#include "tts_driver.h"
#include "main.h"
#include "usart.h"
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

extern UART_HandleTypeDef huart2;

// 内部函数：计算字符串的 GBK 长度
// 由于 STM32F103 缺少完整的 GBK 编码表，这里做简化处理：
// 如果字符 < 0x80，说明是 ASCII，直接计 1
// 中文字符按 2 字节计算（简化假设）
static size_t calc_gbk_len(const char *s) {
    size_t len = 0;
    while (*s) {
        if ((uint8_t)(*s) < 0x80) {
            len++;
        } else {
            len += 2;  // 简化：假设双字节字符
            if (s[1]) s++;
        }
        s++;
    }
    return len;
}

void tts_init(void) {
    // 可以在这里做模块初始化，例如设置默认音量
    // TTS 模块上电后默认可以工作
}

bool tts_speak(const char *text) {
    if (!text || *text == '\0') {
        return false;
    }

    // 计算文本长度（简化处理）
    size_t text_len = strlen(text);  // 实际用 GBK 时需要转换
    size_t frame_len = text_len + 2;  // 帧长度 = 文本 + 2

    // 构建帧头
    uint8_t frame[6] = {
        0xFD,                    // 帧开始
        0x00,                    // 高字节长度（固定0）
        (uint8_t)(frame_len & 0xFF),  // 低字节长度
        0x01,                    // 命令类型（合成播放）
        0x05                     // 文本类型（GBK）
    };

    // 发送帧头
    if (HAL_UART_Transmit(&huart2, frame, 5, 100) != HAL_OK) {
        return false;
    }

    // 发送文本（实际应转为 GBK）
    // 这里直接发送，假设模块能处理或文本是 ASCII
    if (HAL_UART_Transmit((UART_HandleTypeDef*)&huart2, (uint8_t*)text, text_len, 500) != HAL_OK) {
        return false;
    }

    return true;
}

void tts_stop(void) {
    uint8_t cmd[] = {0xFD, 0x00, 0x01, 0x03};
    HAL_UART_Transmit(&huart2, cmd, sizeof(cmd), 100);
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
    HAL_UART_Transmit(&huart2, cmd, sizeof(cmd), 100);
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
    HAL_UART_Transmit(&huart2, cmd, sizeof(cmd), 100);
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
    HAL_UART_Transmit(&huart2, frame, 5, 100);
    HAL_UART_Transmit((UART_HandleTypeDef*)&huart2, (uint8_t*)cmd_str, len, 100);
}