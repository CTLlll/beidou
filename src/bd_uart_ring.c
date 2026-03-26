#include "bd_uart_ring.h"

void bd_ring_init(bd_ring_t *r, uint8_t *storage, size_t storage_len) {
  r->buf = storage;
  r->cap = storage_len;
  r->head = 0;
  r->tail = 0;
}

static size_t next_i(const bd_ring_t *r, size_t i) { return (i + 1u) % r->cap; }

bool bd_ring_push(bd_ring_t *r, uint8_t b) {
  const size_t n = next_i(r, r->head);
  if (n == r->tail) return false; // full
  r->buf[r->head] = b;
  r->head = n;
  return true;
}

bool bd_ring_pop(bd_ring_t *r, uint8_t *out) {
  if (r->tail == r->head) return false; // empty
  *out = r->buf[r->tail];
  r->tail = next_i(r, r->tail);
  return true;
}

size_t bd_ring_available(const bd_ring_t *r) {
  if (r->head >= r->tail) return (size_t)(r->head - r->tail);
  return (size_t)(r->cap - (r->tail - r->head));
}

