#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "bd_nmea.h"
#include "bd_uart_ring.h"
#include "bd_landmarks.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*bd_on_enter_landmark_fn)(const bd_landmark_t *lm, float distance_m, const bd_nmea_position_t *pos);
typedef void (*bd_on_exit_landmark_fn)(const bd_landmark_t *lm, float distance_m, const bd_nmea_position_t *pos);
typedef void (*bd_on_position_update_fn)(const bd_nmea_position_t *pos);

typedef struct {
  bd_on_enter_landmark_fn on_enter;
  bd_on_exit_landmark_fn on_exit;
  bd_on_position_update_fn on_pos;
} bd_app_callbacks_t;

typedef struct {
  bd_ring_t rx;
  bd_nmea_sentence_acc_t acc;
  bd_nmea_position_t pos;

  bd_landmark_db_t db;

  bd_app_callbacks_t cbs;

  // 状态：当前是否在某个地标内
  bool in_landmark;
  uint16_t current_id;
} bd_app_t;

// 初始化：用户提供 RX 环形缓冲区和 NMEA 行缓冲区
void bd_app_init(bd_app_t *app,
                 uint8_t *rx_storage, uint16_t rx_storage_len,
                 char *nmea_line_storage, uint16_t nmea_line_storage_len,
                 bd_landmark_t *landmark_storage, uint16_t landmark_cap,
                 bd_app_callbacks_t cbs);

// 串口接收中断/DMA 回调里逐字节调用
void bd_app_on_uart_rx_byte(bd_app_t *app, uint8_t b);

// 主循环周期性调用（建议 50~200ms）
void bd_app_poll(bd_app_t *app);

// 对外：添加地标
bool bd_app_add_landmark(bd_app_t *app, const bd_landmark_t *lm);
void bd_app_clear_landmarks(bd_app_t *app);

#ifdef __cplusplus
}
#endif

