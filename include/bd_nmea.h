#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "bd_geo.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  BD_NMEA_FIX_NONE = 0,
  BD_NMEA_FIX_VALID = 1,
} bd_nmea_fix_t;

typedef struct {
  bd_nmea_fix_t fix;
  bd_geo_ll_t ll;
  uint8_t sats;     // GGA: satellites in use (0-99), 0 if unknown
  float hdop;       // GGA: HDOP, -1 if unknown
  float altitude_m; // GGA: altitude (mean sea level), 0 if unknown
} bd_nmea_position_t;

typedef struct {
  // sentence buffer (including '$'...'\\n')
  char *buf;
  size_t cap;
  size_t len;
} bd_nmea_sentence_acc_t;

void bd_nmea_sentence_acc_init(bd_nmea_sentence_acc_t *a, char *storage, size_t storage_len);

// 喂入串口字节；当返回 true，表示产出了一条完整 NMEA 行（以 \\n 结束）
bool bd_nmea_sentence_acc_feed(bd_nmea_sentence_acc_t *a, char b);

// 校验 NMEA 校验和（若语句不含 *XX，则返回 false）
bool bd_nmea_checksum_ok(const char *sentence, size_t len);

// 解析 RMC/GGA 获取位置。若成功更新 out（fix/ll 等），返回 true。
bool bd_nmea_parse_position(const char *sentence, size_t len, bd_nmea_position_t *out);

#ifdef __cplusplus
}
#endif

