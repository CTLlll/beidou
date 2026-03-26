#include "bd_geo.h"

#include <math.h>

static double deg2rad(double deg) { return deg * (3.14159265358979323846 / 180.0); }

double bd_geo_distance_m(bd_geo_ll_t a, bd_geo_ll_t b) {
  // WGS84 mean radius (m)
  const double R = 6371008.8;

  const double lat1 = deg2rad(a.lat_deg);
  const double lat2 = deg2rad(b.lat_deg);
  const double dlat = lat2 - lat1;
  const double dlon = deg2rad(b.lon_deg - a.lon_deg);

  const double s1 = sin(dlat / 2.0);
  const double s2 = sin(dlon / 2.0);
  const double h = s1 * s1 + cos(lat1) * cos(lat2) * s2 * s2;

  // clamp for numeric safety
  const double hc = (h < 0.0) ? 0.0 : (h > 1.0 ? 1.0 : h);
  return 2.0 * R * asin(sqrt(hc));
}

