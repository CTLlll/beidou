#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint8_t *buf;
  size_t cap;
  volatile size_t head; // write
  volatile size_t tail; // read
} bd_ring_t;

// 用户提供缓冲区内存（静态数组即可）
void bd_ring_init(bd_ring_t *r, uint8_t *storage, size_t storage_len);

// 写入一个字节；满则返回 false（字节丢弃）
bool bd_ring_push(bd_ring_t *r, uint8_t b);

// 读取一个字节；空则返回 false
bool bd_ring_pop(bd_ring_t *r, uint8_t *out);

// 当前可读字节数
size_t bd_ring_available(const bd_ring_t *r);

#ifdef __cplusplus
}
#endif

