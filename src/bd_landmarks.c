#include "bd_landmarks.h"

#include <string.h>

void bd_landmark_db_init(bd_landmark_db_t *db, bd_landmark_t *storage, uint16_t storage_cap) {
  db->items = storage;
  db->cap = storage_cap;
  db->count = 0;
}

bool bd_landmark_db_add(bd_landmark_db_t *db, const bd_landmark_t *lm) {
  if (!db || !lm) return false;
  if (db->count >= db->cap) return false;
  db->items[db->count++] = *lm;
  return true;
}

void bd_landmark_db_clear(bd_landmark_db_t *db) {
  if (!db) return;
  db->count = 0;
}

