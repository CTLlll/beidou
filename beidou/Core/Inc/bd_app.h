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

  /** false：只解析显示定位，不判地标、不触发 on_enter/on_exit */
  bool landmark_enabled;

  /** NMEA 解析统计（主循环 bd_app_poll 更新，供屏显） */
  uint32_t stat_nmea_lines;    /* 收到完整 NMEA 行（以换行结束） */
  uint32_t stat_checksum_ok;   /* 校验和通过 */
  uint32_t stat_checksum_bad;  /* 校验和失败 */
  uint32_t stat_rmc;           /* 校验通过后识别为 RMC */
  uint32_t stat_gga;           /* 校验通过后识别为 GGA */
  uint32_t stat_other_nmea;    /* 校验通过、其它 $ 句 */
  uint32_t stat_parse_ok;      /* bd_nmea_parse_position 成功（含无定位的 RMC/GGA） */
  uint32_t stat_parse_skip;    /* 校验通过但 parse 失败（字段异常等） */
} bd_app_t;

// 初始化：用户提供 RX 环形缓冲区和 NMEA 行缓冲区
void bd_app_init(bd_app_t *app,
                 uint8_t *rx_storage, uint16_t rx_storage_len,
                 char *nmea_line_storage, uint16_t nmea_line_storage_len,
                 bd_landmark_t *landmark_storage, uint16_t landmark_cap,
                 bd_app_callbacks_t cbs);

// 串口接收中断/DMA 回调里逐字节调用
void bd_app_on_uart_rx_byte(bd_app_t *app, uint8_t b);

/**
 * 主循环周期性调用（建议 50~200ms）。
 * 每轮最多从环形缓冲取出 BD_APP_POLL_MAX_BYTES 字节再解析，避免单轮占用过长；
 * NMEA 约 1Hz、115200 下通常远小于此值。
 */
#ifndef BD_APP_POLL_MAX_BYTES
#define BD_APP_POLL_MAX_BYTES 512u
#endif
void bd_app_poll(bd_app_t *app);

// 对外：添加地标
bool bd_app_add_landmark(bd_app_t *app, const bd_landmark_t *lm);
void bd_app_clear_landmarks(bd_app_t *app);

/** 启停地标判定；切换时清除 in_landmark，避免关闭期间状态粘连 */
void bd_app_set_landmark_enabled(bd_app_t *app, bool enabled);

#ifdef __cplusplus
}
#endif

