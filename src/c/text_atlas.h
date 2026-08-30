#pragma once

#include <pebble.h>

#include "generated/glyphs.h"

typedef struct {
  int16_t width;
  int16_t offset_y;
  int16_t height;
  int16_t ink_left;
  int16_t ink_w;
} TAMetrics;

void ta_measure(const Atlas *main, const Atlas *special, char special_ch,
                const char *s, TAMetrics *m);

int16_t ta_draw_mixed(GContext *ctx, const Atlas *main, const Atlas *special,
                      char special_ch, const char *s, int16_t x, int16_t y,
                      GColor color, GColor bg, int16_t clip_right);

void ta_draw(GContext *ctx, const Atlas *atlas, const char *s, int16_t x,
             int16_t y, GColor color, GColor bg, int16_t clip_right);

void ta_center(GContext *ctx, const Atlas *main, const Atlas *special,
               char special_ch, const char *s, GRect box, GColor color,
               GColor bg);

void ta_draw_static_line(GContext *ctx, const StaticLine *line, int16_t x,
                         int16_t y, GColor color, GColor bg);

void ta_fill_circle_band(int16_t y0, int16_t y1, GColor color);

typedef struct {
  const Atlas *atlas;
  const StaticLine *line;
  const char *text;
  int8_t gap_before;
} TaArcPiece;

uint16_t ta_arc_pieces_width(const TaArcPiece *pieces, uint8_t count);

void ta_draw_arc(GContext *ctx, const TaArcPiece *pieces, uint8_t count,
                 int16_t cx, int16_t cy, uint16_t radius, GColor color,
                 GColor bg);

void ta_set_clip(const GRect *clip);

bool ta_begin_frame(GContext *ctx);
void ta_end_frame(GContext *ctx);

void ta_fill_rect(GRect r, GColor color);
