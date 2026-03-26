#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "bd_geo.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint16_t id;
  bd_geo_ll_t ll;
  float enter_radius_m;   // 进入阈值（米）
  float exit_radius_m;    // 离开阈值（米），用于滞回；若为 0 则使用 enter_radius_m * 1.3
  const char *title;      // 屏幕标题（可选）
  const char *speech_text;// 语音/讲解文本（可选）
} bd_landmark_t;

typedef struct {
  bd_landmark_t *items;
  uint16_t cap;
  uint16_t count;
} bd_landmark_db_t;

void bd_landmark_db_init(bd_landmark_db_t *db, bd_landmark_t *storage, uint16_t storage_cap);
bool bd_landmark_db_add(bd_landmark_db_t *db, const bd_landmark_t *lm);
void bd_landmark_db_clear(bd_landmark_db_t *db);

#ifdef __cplusplus
}
#endif

