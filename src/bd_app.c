#include "bd_app.h"

#include <string.h>

#include "bd_geo.h"

static float lm_exit_radius(const bd_landmark_t *lm) {
  if (lm->exit_radius_m > 0.01f) return lm->exit_radius_m;
  return lm->enter_radius_m * 1.3f;
}

void bd_app_init(bd_app_t *app,
                 uint8_t *rx_storage, uint16_t rx_storage_len,
                 char *nmea_line_storage, uint16_t nmea_line_storage_len,
                 bd_landmark_t *landmark_storage, uint16_t landmark_cap,
                 bd_app_callbacks_t cbs) {
  memset(app, 0, sizeof(*app));

  bd_ring_init(&app->rx, rx_storage, rx_storage_len);
  bd_nmea_sentence_acc_init(&app->acc, nmea_line_storage, nmea_line_storage_len);
  bd_landmark_db_init(&app->db, landmark_storage, landmark_cap);

  app->pos.fix = BD_NMEA_FIX_NONE;
  app->in_landmark = false;
  app->current_id = 0;

  app->cbs = cbs;
}

void bd_app_on_uart_rx_byte(bd_app_t *app, uint8_t b) {
  (void)bd_ring_push(&app->rx, b);
}

bool bd_app_add_landmark(bd_app_t *app, const bd_landmark_t *lm) {
  return bd_landmark_db_add(&app->db, lm);
}

void bd_app_clear_landmarks(bd_app_t *app) {
  bd_landmark_db_clear(&app->db);
  app->in_landmark = false;
  app->current_id = 0;
}

static const bd_landmark_t *find_by_id(const bd_landmark_db_t *db, uint16_t id) {
  for (uint16_t i = 0; i < db->count; i++) {
    if (db->items[i].id == id) return &db->items[i];
  }
  return NULL;
}

static const bd_landmark_t *pick_enter_candidate(const bd_landmark_db_t *db,
                                                 bd_geo_ll_t ll,
                                                 float *out_distance_m) {
  const bd_landmark_t *best = NULL;
  float best_d = 0.0f;

  for (uint16_t i = 0; i < db->count; i++) {
    const bd_landmark_t *lm = &db->items[i];
    const float d = (float)bd_geo_distance_m(ll, lm->ll);
    if (d <= lm->enter_radius_m) {
      if (!best || d < best_d) {
        best = lm;
        best_d = d;
      }
    }
  }

  if (best && out_distance_m) *out_distance_m = best_d;
  return best;
}

void bd_app_poll(bd_app_t *app) {
  uint8_t b = 0;
  while (bd_ring_pop(&app->rx, &b)) {
    if (!bd_nmea_sentence_acc_feed(&app->acc, (char)b)) continue;

    const char *line = app->acc.buf;
    const size_t len = app->acc.len;

    if (!bd_nmea_checksum_ok(line, len)) continue;

    bd_nmea_position_t newpos = app->pos;
    if (!bd_nmea_parse_position(line, len, &newpos)) continue;

    app->pos = newpos;
    if (app->cbs.on_pos) app->cbs.on_pos(&app->pos);

    if (app->pos.fix != BD_NMEA_FIX_VALID) continue;

    // state machine with hysteresis
    if (!app->in_landmark) {
      float d_enter = 0.0f;
      const bd_landmark_t *lm = pick_enter_candidate(&app->db, app->pos.ll, &d_enter);
      if (lm) {
        app->in_landmark = true;
        app->current_id = lm->id;
        if (app->cbs.on_enter) app->cbs.on_enter(lm, d_enter, &app->pos);
      }
    } else {
      const bd_landmark_t *cur = find_by_id(&app->db, app->current_id);
      if (!cur) {
        app->in_landmark = false;
        app->current_id = 0;
        continue;
      }
      const float d = (float)bd_geo_distance_m(app->pos.ll, cur->ll);
      if (d >= lm_exit_radius(cur)) {
        app->in_landmark = false;
        app->current_id = 0;
        if (app->cbs.on_exit) app->cbs.on_exit(cur, d, &app->pos);
      }
    }
  }
}

