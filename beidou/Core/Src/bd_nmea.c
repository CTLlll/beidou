#include "bd_nmea.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static int hex2i(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  return -1;
}

void bd_nmea_sentence_acc_init(bd_nmea_sentence_acc_t *a, char *storage, size_t storage_len) {
  a->buf = storage;
  a->cap = storage_len;
  a->len = 0;
}

bool bd_nmea_sentence_acc_feed(bd_nmea_sentence_acc_t *a, char b) {
  if (b == '$') {
    a->len = 0;
  }

  if (a->len + 1u < a->cap) {
    a->buf[a->len++] = b;
    a->buf[a->len] = '\0';
  } else {
    // overflow -> reset until next '$'
    a->len = 0;
    return false;
  }

  return (b == '\n' && a->len > 6);
}

bool bd_nmea_checksum_ok(const char *sentence, size_t len) {
  // expects: $.....*HH\r\n
  if (!sentence || len < 9) return false;
  if (sentence[0] != '$') return false;

  const char *star = NULL;
  for (size_t i = 0; i < len; i++) {
    if (sentence[i] == '*') {
      star = &sentence[i];
      break;
    }
    if (sentence[i] == '\r' || sentence[i] == '\n') break;
  }
  if (!star) return false;

  uint8_t cs = 0;
  for (const char *p = sentence + 1; p < star; p++) cs ^= (uint8_t)(*p);

  if ((star + 2) >= (sentence + len)) return false;
  const int h1 = hex2i(star[1]);
  const int h2 = hex2i(star[2]);
  if (h1 < 0 || h2 < 0) return false;
  const uint8_t expected = (uint8_t)((h1 << 4) | h2);
  return cs == expected;
}

static bool parse_degmin(const char *s, bool is_lat, double *out_deg) {
  // lat: ddmm.mmmm, lon: dddmm.mmmm
  if (!s || !*s) return false;
  const int deg_digits = is_lat ? 2 : 3;
  int digits = 0;
  while (isdigit((unsigned char)s[digits]) && digits < deg_digits + 2) digits++;
  if (digits < deg_digits + 2) return false;

  char deg_buf[4] = {0};
  if ((size_t)deg_digits >= sizeof(deg_buf)) return false;
  memcpy(deg_buf, s, (size_t)deg_digits);
  const int deg = atoi(deg_buf);

  const double minutes = atof(s + deg_digits);
  *out_deg = (double)deg + minutes / 60.0;
  return true;
}

static size_t split_fields(char *s, char *fields[], size_t max_fields) {
  size_t n = 0;
  char *p = s;
  while (p && n < max_fields) {
    fields[n++] = p;
    char *comma = strchr(p, ',');
    if (!comma) break;
    *comma = '\0';
    p = comma + 1;
  }
  return n;
}

static bool starts_with(const char *s, const char *prefix) {
  return strncmp(s, prefix, strlen(prefix)) == 0;
}

bool bd_nmea_parse_position(const char *sentence, size_t len, bd_nmea_position_t *out) {
  if (!sentence || !out || len < 10) return false;
  if (sentence[0] != '$') return false;

  // copy up to '*' or CR/LF into a temp buffer for tokenization
  char tmp[128];
  size_t n = 0;
  for (size_t i = 0; i < len && n + 1u < sizeof(tmp); i++) {
    char c = sentence[i];
    if (c == '*') break;
    if (c == '\r' || c == '\n') break;
    tmp[n++] = c;
  }
  tmp[n] = '\0';
  if (n < 6) return false;

  // tmp starts with '$'
  char *fields[24] = {0};
  size_t nf = split_fields(tmp, fields, 24);
  if (nf < 2) return false;

  const char *type = fields[0]; // e.g. "$GNRMC"

  // RMC: $--RMC,time,status,lat,N,lon,E,speed,course,date,...
  if (starts_with(type, "$GPRMC") || starts_with(type, "$GNRMC") || starts_with(type, "$BDRMC")) {
    if (nf < 7) return false;
    const char *status = fields[2]; // A=valid, V=void
    const char *lat_s = fields[3];
    const char *lat_h = fields[4];
    const char *lon_s = fields[5];
    const char *lon_h = fields[6];

    if (!status || status[0] != 'A') {
      out->fix = BD_NMEA_FIX_NONE;
      return true;
    }

    double lat = 0.0, lon = 0.0;
    if (!parse_degmin(lat_s, true, &lat)) return false;
    if (!parse_degmin(lon_s, false, &lon)) return false;
    if (lat_h && (lat_h[0] == 'S' || lat_h[0] == 's')) lat = -lat;
    if (lon_h && (lon_h[0] == 'W' || lon_h[0] == 'w')) lon = -lon;

    out->fix = BD_NMEA_FIX_VALID;
    out->ll.lat_deg = lat;
    out->ll.lon_deg = lon;
    return true;
  }

  // GGA: $--GGA,time,lat,N,lon,E,fix,numsats,hdop,alt,M,...
  if (starts_with(type, "$GPGGA") || starts_with(type, "$GNGGA") || starts_with(type, "$BDGGA")) {
    if (nf < 10) return false;
    const char *lat_s = fields[2];
    const char *lat_h = fields[3];
    const char *lon_s = fields[4];
    const char *lon_h = fields[5];
    const char *fix_q = fields[6];   // 0 invalid, 1 GPS, 2 DGPS, 4 RTK...
    const char *sats_s = fields[7];
    const char *hdop_s = fields[8];
    const char *alt_s = fields[9];

    const int fix_i = fix_q ? atoi(fix_q) : 0;
    out->sats = (uint8_t)(sats_s ? atoi(sats_s) : 0);
    out->hdop = (hdop_s && *hdop_s) ? (float)atof(hdop_s) : -1.0f;
    out->altitude_m = (alt_s && *alt_s) ? (float)atof(alt_s) : 0.0f;

    if (fix_i <= 0) {
      out->fix = BD_NMEA_FIX_NONE;
      return true;
    }

    double lat = 0.0, lon = 0.0;
    if (!parse_degmin(lat_s, true, &lat)) return false;
    if (!parse_degmin(lon_s, false, &lon)) return false;
    if (lat_h && (lat_h[0] == 'S' || lat_h[0] == 's')) lat = -lat;
    if (lon_h && (lon_h[0] == 'W' || lon_h[0] == 'w')) lon = -lon;

    out->fix = BD_NMEA_FIX_VALID;
    out->ll.lat_deg = lat;
    out->ll.lon_deg = lon;
    return true;
  }

  return false;
}

