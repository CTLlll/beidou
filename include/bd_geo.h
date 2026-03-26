#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  double lat_deg;  // +北纬, -南纬
  double lon_deg;  // +东经, -西经
} bd_geo_ll_t;

// 以 WGS84 近似球体半径计算（哈弗辛公式）。返回：米
double bd_geo_distance_m(bd_geo_ll_t a, bd_geo_ll_t b);

#ifdef __cplusplus
}
#endif

