#include "text_atlas.h"

#include <math.h>
#include <string.h>

static const Glyph *meta_of(const Atlas *atlas, char c) {
  uint8_t u = (uint8_t)c;
  if (u < 32 || u > 126) {
    return NULL;
  }
  uint8_t idx = atlas->map[u - 32];
  if (idx == 0xFF) {
    return NULL;
  }
  return &atlas->glyphs[idx];
}

static GRect s_clip;
static bool s_clip_on;

typedef struct {
  uint8_t *data;
  uint16_t stride;
  int16_t w, h;
} FBSurface;

static GBitmap *s_fb_bmp;
static FBSurface s_fb;
static bool s_fb_active;
static bool s_round_fb;

static int16_t round_lo(int16_t py) {
  if (!s_round_fb || !s_fb_active || py < 0 || py >= s_fb.h) {
    return 0;
  }
  GBitmapDataRowInfo ri = gbitmap_get_data_row_info(s_fb_bmp, (uint16_t)py);
  return ri.min_x;
}

static int16_t round_hi(int16_t py) {
  if (!s_round_fb || !s_fb_active || py < 0 || py >= s_fb.h) {
    return (int16_t)(s_fb.w - 1);
  }
  GBitmapDataRowInfo ri = gbitmap_get_data_row_info(s_fb_bmp, (uint16_t)py);
  return ri.max_x;
}

static uint8_t *row_ptr(int16_t py, int16_t *lo, int16_t *hi) {
  if (!s_fb_active || py < 0 || py >= s_fb.h) {
    return NULL;
  }
  if (s_round_fb) {
    GBitmapDataRowInfo ri =
        gbitmap_get_data_row_info(s_fb_bmp, (uint16_t)py);
    if (ri.max_x < ri.min_x) {
      return NULL;
    }
    *lo = ri.min_x;
    *hi = ri.max_x + 1;
    return ri.data;
  }
  *lo = 0;
  *hi = s_fb.w;
  return s_fb.data + (size_t)py * s_fb.stride;
}

void ta_set_clip(const GRect *clip) {
  if (clip) {
    s_clip = *clip;
    s_clip_on = true;
  } else {
    s_clip_on = false;
  }
}

bool ta_begin_frame(GContext *ctx) {
  s_fb_bmp = graphics_capture_frame_buffer_format(ctx, GBitmapFormat8Bit);
  if (!s_fb_bmp) {
    s_fb_bmp =
        graphics_capture_frame_buffer_format(ctx, GBitmapFormat8BitCircular);
  }
  if (!s_fb_bmp) {
    s_fb_active = false;
    return false;
  }
  s_fb.data = gbitmap_get_data(s_fb_bmp);
  s_fb.stride = gbitmap_get_bytes_per_row(s_fb_bmp);
  const GRect b = gbitmap_get_bounds(s_fb_bmp);
  s_fb.w = b.size.w;
  s_fb.h = b.size.h;
  s_round_fb =
      (gbitmap_get_format(s_fb_bmp) == GBitmapFormat8BitCircular);
  s_fb_active = true;
  return true;
}

void ta_end_frame(GContext *ctx) {
  if (s_fb_active) {
    graphics_release_frame_buffer(ctx, s_fb_bmp);
    s_fb_active = false;
  }
}

void ta_fill_rect(GRect r, GColor color) {
  if (!s_fb_active) {
    return;
  }
  int16_t x0 = r.origin.x, y0 = r.origin.y;
  int16_t x1 = r.origin.x + r.size.w, y1 = r.origin.y + r.size.h;
  if (s_clip_on) {
    if (x0 < s_clip.origin.x) x0 = s_clip.origin.x;
    if (y0 < s_clip.origin.y) y0 = s_clip.origin.y;
    if (x1 > s_clip.origin.x + s_clip.size.w)
      x1 = s_clip.origin.x + s_clip.size.w;
    if (y1 > s_clip.origin.y + s_clip.size.h)
      y1 = s_clip.origin.y + s_clip.size.h;
  }
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > s_fb.w) x1 = s_fb.w;
  if (y1 > s_fb.h) y1 = s_fb.h;
  for (int16_t py = y0; py < y1; py++) {
    int16_t lo, hi;
    uint8_t *row = row_ptr(py, &lo, &hi);
    if (!row) {
      continue;
    }
    if (lo < x0) lo = x0;
    if (hi > x1) hi = x1;
    if (s_clip_on) {
      if (lo < s_clip.origin.x) lo = s_clip.origin.x;
      if (hi > s_clip.origin.x + s_clip.size.w)
        hi = s_clip.origin.x + s_clip.size.w;
    }
    if (lo < hi) {
      memset(row + lo, color.argb, hi - lo);
    }
  }
}

void ta_fill_circle_band(int16_t y0, int16_t y1, GColor color) {
  if (!s_fb_active) {
    return;
  }
  if (s_clip_on) {
    if (y0 < s_clip.origin.y) y0 = s_clip.origin.y;
    if (y1 > s_clip.origin.y + s_clip.size.h)
      y1 = s_clip.origin.y + s_clip.size.h;
  }
  if (y0 < 0) y0 = 0;
  if (y1 > s_fb.h) y1 = s_fb.h;
  for (int16_t py = y0; py < y1; py++) {
    int16_t lo, hi;
    uint8_t *row = row_ptr(py, &lo, &hi);
    if (!row) {
      continue;
    }
    if (s_clip_on) {
      if (lo < s_clip.origin.x) lo = s_clip.origin.x;
      if (hi > s_clip.origin.x + s_clip.size.w)
        hi = s_clip.origin.x + s_clip.size.w;
    }
    if (lo < hi) {
      memset(row + lo, color.argb, hi - lo);
    }
  }
}

static void build_lut(GColor bg, GColor ink, GColor lut[4]) {
  for (int l = 1; l <= 3; l++) {
    GColor c;
    c.a = 3;
    c.r = (uint8_t)((bg.r * (3 - l) + ink.r * l + 1) / 3);
    c.g = (uint8_t)((bg.g * (3 - l) + ink.g * l + 1) / 3);
    c.b = (uint8_t)((bg.b * (3 - l) + ink.b * l + 1) / 3);
    lut[l] = c;
  }
}

static void write_levels(const uint8_t *bits, uint16_t w, uint16_t h,
                         int16_t x, int16_t y, const GColor lut[4]) {
  int16_t cx0 = (x < 0) ? 0 : x;
  int16_t cy0 = (y < 0) ? 0 : y;
  int16_t cx1 = (x + w > s_fb.w) ? s_fb.w : x + w;
  int16_t cy1 = (y + h > s_fb.h) ? s_fb.h : y + h;

  if (s_clip_on) {
    if (cx0 < s_clip.origin.x) cx0 = s_clip.origin.x;
    if (cy0 < s_clip.origin.y) cy0 = s_clip.origin.y;
    if (cx1 > s_clip.origin.x + s_clip.size.w)
      cx1 = s_clip.origin.x + s_clip.size.w;
    if (cy1 > s_clip.origin.y + s_clip.size.h)
      cy1 = s_clip.origin.y + s_clip.size.h;
  }
  if (cx0 >= cx1 || cy0 >= cy1) {
    return;
  }

  const uint16_t src_stride = (w + 3) / 4;

  for (int16_t py = cy0; py < cy1; py++) {
    const uint16_t sy = py - y;
    const uint8_t *src = bits + (size_t)sy * src_stride;
    int16_t lo, hi;
    uint8_t *dst = row_ptr(py, &lo, &hi);
    if (!dst) {
      continue;
    }
    if (lo < cx0) lo = cx0;
    if (hi > cx1) hi = cx1;
    if (s_clip_on) {
      if (lo < s_clip.origin.x) lo = s_clip.origin.x;
      if (hi > s_clip.origin.x + s_clip.size.w)
        hi = s_clip.origin.x + s_clip.size.w;
    }
    for (int16_t pxx = lo; pxx < hi; pxx++) {
      const uint16_t sx = pxx - x;
      const uint8_t lvl = (src[sx >> 2] >> (6 - 2 * (sx & 3))) & 3;
      if (lvl) {
        dst[pxx] = lut[lvl].argb;
      }
    }
  }
}

int16_t ta_draw_mixed(GContext *ctx, const Atlas *main, const Atlas *special,
                      char special_ch, const char *s, int16_t x, int16_t y,
                      GColor color, GColor bg, int16_t clip_right) {
  (void)ctx;
  if (!s_fb_active) {
    return x;
  }
  const int16_t baseline = y + main->ascent;
  GColor lut[4];
  build_lut(bg, color, lut);

  for (; *s; s++) {
    if (clip_right >= 0 && x >= clip_right) {
      break;
    }
    const Atlas *a = (special && *s == special_ch) ? special : main;
    const Glyph *g = meta_of(a, *s);
    if (!g || g->w == 0 || g->h == 0) {
      if (g) {
        x += g->advance;
      }
      continue;
    }
    write_levels(a->data + g->offset, g->w, g->h, x + g->left,
                 baseline + g->top, lut);
    x += g->advance;
  }
  return x;
}

void ta_draw(GContext *ctx, const Atlas *atlas, const char *s, int16_t x,
             int16_t y, GColor color, GColor bg, int16_t clip_right) {
  ta_draw_mixed(ctx, atlas, NULL, 0, s, x, y, color, bg, clip_right);
}

void ta_measure(const Atlas *main, const Atlas *special, char special_ch,
                const char *s, TAMetrics *m) {
  m->width = 0;
  m->height = 0;
  int16_t min_top = INT16_MAX;
  int16_t max_bot = 0;
  int16_t min_left = INT16_MAX;
  int16_t max_right = 0;
  int16_t pen = 0;
  for (; *s; s++) {
    const Atlas *a = (special && *s == special_ch) ? special : main;
    const Glyph *g = meta_of(a, *s);
    if (!g) {
      continue;
    }
    if (g->h != 0) {
      if (pen + g->left < min_left) {
        min_left = pen + g->left;
      }
      if (pen + g->left + g->w > max_right) {
        max_right = pen + g->left + g->w;
      }
      if (g->top < min_top) {
        min_top = g->top;
      }
      if (g->top + g->h > max_bot) {
        max_bot = g->top + g->h;
      }
    }
    pen += g->advance;
  }
  m->width = pen;
  if (min_top == INT16_MAX) {
    min_top = 0;
  }
  m->offset_y = min_top;
  m->height = max_bot - min_top;
  if (min_left == INT16_MAX) {
    min_left = 0;
    max_right = 0;
  }
  m->ink_left = min_left;
  m->ink_w = max_right - min_left;
}

void ta_center(GContext *ctx, const Atlas *main, const Atlas *special,
               char special_ch, const char *s, GRect box, GColor color,
               GColor bg) {
  TAMetrics m;
  ta_measure(main, special, special_ch, s, &m);
  int16_t x = box.origin.x + (box.size.w - m.width) / 2;
  int16_t y = box.origin.y + (box.size.h - m.height) / 2 - m.offset_y
              - main->ascent;
  ta_draw_mixed(ctx, main, special, special_ch, s, x, y, color, bg,
                box.origin.x + box.size.w);
}

void ta_draw_static_line(GContext *ctx, const StaticLine *line, int16_t x,
                         int16_t y, GColor color, GColor bg) {
  if (!s_fb_active) {
    return;
  }
  GColor lut[4];
  build_lut(bg, color, lut);
  write_levels(line->data, line->w, line->h, x, y, lut);
}

uint16_t ta_arc_pieces_width(const TaArcPiece *pieces, uint8_t count) {
  uint16_t total = 0;
  for (uint8_t i = 0; i < count; i++) {
    total += pieces[i].gap_before;
    if (pieces[i].line) {
      total += pieces[i].line->w;
    } else if (pieces[i].atlas && pieces[i].text) {
      TAMetrics m;
      ta_measure(pieces[i].atlas, NULL, 0, pieces[i].text, &m);
      total += m.width;
    }
  }
  return total;
}

static void blit_rotated(const uint8_t *bits, uint16_t w, uint16_t h,
                         float bx, float by, int16_t ax, int16_t ay,
                         float ca, float sa, const GColor lut[4]) {
  const float c[4][2] = {{-bx, -by}, {w - bx, -by}, {-bx, h - by},
                         {w - bx, h - by}};
  float minx = 0, maxx = 0, miny = 0, maxy = 0;
  for (int i = 0; i < 4; i++) {
    const float rx = c[i][0] * ca - c[i][1] * sa;
    const float ry = c[i][0] * sa + c[i][1] * ca;
    if (rx < minx) minx = rx;
    if (rx > maxx) maxx = rx;
    if (ry < miny) miny = ry;
    if (ry > maxy) maxy = ry;
  }
  int16_t X0 = ax + (int16_t)(minx - 1);
  int16_t X1 = ax + (int16_t)(maxx + 2);
  int16_t Y0 = ay + (int16_t)(miny - 1);
  int16_t Y1 = ay + (int16_t)(maxy + 2);

  const uint16_t stride = (w + 3) / 4;
  for (int16_t py = Y0; py < Y1; py++) {
    const float dy = py - ay;
    int16_t lo, hi;
    uint8_t *dst = row_ptr(py, &lo, &hi);
    if (!dst) {
      continue;
    }
    if (lo < X0) lo = X0;
    if (hi > X1) hi = X1;
    if (s_clip_on) {
      if (lo < s_clip.origin.x) lo = s_clip.origin.x;
      if (hi > s_clip.origin.x + s_clip.size.w)
        hi = s_clip.origin.x + s_clip.size.w;
    }
    for (int16_t pxx = lo; pxx < hi; pxx++) {
      const float dx = pxx - ax;
      const float u = bx + dx * ca + dy * sa;
      const float v = by - dx * sa + dy * ca;
      const int16_t ui = (int16_t)u;
      const int16_t vi = (int16_t)v;
      if (ui < 0 || ui >= (int16_t)w || vi < 0 || vi >= (int16_t)h) {
        continue;
      }
      const uint8_t lvl =
          (bits[(size_t)vi * stride + (ui >> 2)] >> (6 - 2 * (ui & 3))) & 3;
      if (lvl) {
        dst[pxx] = lut[lvl].argb;
      }
    }
  }
}

void ta_draw_arc(GContext *ctx, const TaArcPiece *pieces, uint8_t count,
                 int16_t cx, int16_t cy, uint16_t radius, GColor color,
                 GColor bg) {
  (void)ctx;
  if (!s_fb_active || radius == 0) {
    return;
  }
  const uint16_t total = ta_arc_pieces_width(pieces, count);
  if (!total) {
    return;
  }
  GColor lut[4];
  build_lut(bg, color, lut);

  const float theta_total = (float)total / (float)radius;
  float theta = -theta_total / 2.0f;

  for (uint8_t i = 0; i < count; i++) {
    theta += (float)pieces[i].gap_before / (float)radius;
    if (pieces[i].line) {
      const StaticLine *ln = pieces[i].line;
      const float byp = ln->h * 0.8f;
      const float mid = theta +
                        ((float)ln->w / (float)radius) / 2.0f;
      const int16_t ax = cx + (int16_t)(sinf(mid) * radius + 0.5f);
      const int16_t ay = cy - (int16_t)(cosf(mid) * radius + 0.5f);
      blit_rotated(ln->data, ln->w, ln->h, ln->w / 2.0f, byp, ax, ay,
                   cosf(mid), sinf(mid), lut);
      theta += ((float)ln->w / (float)radius);
    } else if (pieces[i].atlas && pieces[i].text) {
      const Atlas *a = pieces[i].atlas;
      for (const char *s = pieces[i].text; *s; s++) {
        const Glyph *g = meta_of(a, *s);
        if (!g || g->w == 0 || g->h == 0) {
          if (g) {
            theta += (float)g->advance / (float)radius;
          }
          continue;
        }
        const float step = (float)g->advance / (float)radius;
        const float mid = theta + step / 2.0f;
        const int16_t ax = cx + (int16_t)(sinf(mid) * radius + 0.5f);
        const int16_t ay = cy - (int16_t)(cosf(mid) * radius + 0.5f);
        blit_rotated(a->data + g->offset, g->w, g->h, g->w / 2.0f,
                     (float)(-g->top), ax, ay, cosf(mid), sinf(mid), lut);
        theta += step;
      }
    }
  }
}
