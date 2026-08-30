#include "ui.h"

#include <stdio.h>

#include "anim.h"
#include "keys.h"
#include "text_atlas.h"
#include "generated/glyphs.h"

#define FOOTER_NONE 0
#define FOOTER_LINE 1
#define FOOTER_ARC 2

#if defined(PBL_PLATFORM_CHALK)

#define SCREEN_W 180
#define SCREEN_H 180
#define SOY_ROUND 1
#define CAP_H 37

#define DAY_X 15
#define DAY_Y 37

#define FLAVOR_ATLAS g_atlas_flavor12
#define FLAVOR_CENTERED 1
#define FLAVOR_BOX GRect(30, 131, 120, 16)

#define SOY_NO_BOX 1

static const GRect CHIP_A_N = GRect(23, 66, 70, 61);
static const GRect MINUTE_NOMINAL_N = GRect(94, 66, 70, 61);

static const GRect CHIP_A_B = GRect(23, 66, 70, 61);
static const GRect CHIP_B_B = GRect(94, 66, 70, 61);

#define FOOTER_STYLE FOOTER_ARC
#define ARC_CX 90
#define ARC_CY 103
#define ARC_R 84

#elif defined(PBL_PLATFORM_GABBRO)

#define SCREEN_W 260
#define SCREEN_H 260
#define SOY_ROUND 1
#define CAP_H 55

#define DAY_X 47
#define DAY_Y 52

#define FLAVOR_X 47
#define FLAVOR_Y 81
#define FLAVOR_ATLAS g_atlas_flavor12

#define BRAND_BOX_X 58
#define BRAND_BOX_Y 102
#define BRAND_BOX_W 143
#define BRAND_BOX_H_NORMAL 56
#define BRAND_STROKE_NORMAL 3

#define BRAND_MAIN g_atlas_brand32
#define BRAND_E g_atlas_brand_e32

static const GRect CHIP_A_N = GRect(52, 165, 70, 61);
static const GRect MINUTE_NOMINAL_N = GRect(138, 165, 70, 61);

static const GRect CHIP_A_B = GRect(52, 165, 70, 61);
static const GRect CHIP_B_B = GRect(138, 165, 70, 61);

#define FOOTER_STYLE FOOTER_ARC
#define ARC_CX 130
#define ARC_CY 110
#define ARC_R 96

#elif defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_APLITE)

#define SCREEN_W 144
#define SCREEN_H 168
#define BAR_H 16

#define DAY_X 8
#define DAY_Y 13

#define FLAVOR_X 8
#define FLAVOR_Y 40
#define FLAVOR_ATLAS g_atlas_rg9

#define BRAND_BOX_X 8
#define BRAND_BOX_Y 54
#define BRAND_BOX_W 128
#define BRAND_BOX_H_NORMAL 50
#define BRAND_STROKE_NORMAL 3

#define BRAND_MAIN g_atlas_brand32
#define BRAND_E g_atlas_brand_e32

static const GRect CHIP_A_N = GRect(1, 107, 70, 61);
static const GRect MINUTE_NOMINAL_N = GRect(74, 107, 70, 61);

static const GRect CHIP_A_B = GRect(1, 107, 70, 61);
static const GRect CHIP_B_B = GRect(74, 107, 70, 61);

#define FOOTER_STYLE FOOTER_NONE

#else  // emery, core 2 duo. the original, untouched

#define SCREEN_W 200
#define SCREEN_H 228
#define BAR_H 16

#define DAY_X 14
#define DAY_Y 13

#define FLAVOR_X 14
#define FLAVOR_Y 40
#define FLAVOR_ATLAS g_atlas_flavor14

#define BRAND_BOX_X 12
#define BRAND_BOX_Y 58
#define BRAND_BOX_W 176
#define BRAND_BOX_H_NORMAL 83
#define BRAND_BOX_H_BIG 69
#define BRAND_STROKE_NORMAL 4
#define BRAND_STROKE_BIG 3

static const GRect CHIP_A_N = GRect(22, 151, 36, 27);
static const GPoint LABEL_PROTEIN_POS = {64, 156};
static const GRect MINUTE_NOMINAL_N = GRect(22, 180, 36, 27);
static const GPoint LABEL_MINUTES_POS = {64, 187};

static const GRect CHIP_A_B = GRect(16, 141, 80, 60);
static const GRect CHIP_B_B = GRect(103, 141, 80, 60);

#define FOOTER_STYLE FOOTER_LINE
#define FOOTER_Y 211

#define CHIP_VAL_NORMAL g_atlas_val20
#define BRAND_MAIN g_atlas_brand48
#define BRAND_E g_atlas_brand_e48
#define BRAND_FALLBACK g_atlas_brand40
#define BRAND_E_FALLBACK g_atlas_brand_e40
#define SOY_HAS_CHIP_LABELS 1

#endif

#ifndef BRAND_MAIN
#define BRAND_MAIN g_atlas_brand40
#endif
#ifndef BRAND_E
#define BRAND_E g_atlas_brand_e40
#endif
#ifndef BRAND_FALLBACK
#define BRAND_FALLBACK g_atlas_brand40
#endif
#ifndef BRAND_E_FALLBACK
#define BRAND_E_FALLBACK g_atlas_brand_e40
#endif
#ifndef CHIP_VAL_NORMAL
#define CHIP_VAL_NORMAL g_atlas_val36
#endif

#ifndef SOY_ROUND
#define SOY_ROUND 0
#endif

#ifndef CAP_H
#define CAP_H BAR_H
#endif

#ifndef BRAND_BOX_H_BIG
#define BRAND_BOX_H_BIG BRAND_BOX_H_NORMAL
#endif
#ifndef BRAND_STROKE_BIG
#define BRAND_STROKE_BIG BRAND_STROKE_NORMAL
#endif

#if !defined(BRAND_BOX_X)
#define SOY_NO_BOX 1
#endif

static Window *s_window;
SoyState soy_state;

static const char *const DAY_PUNS[7] = {
    "Sundapple", "Mondate", "Tuesberry", "Wedmelon",
    "Thurspear", "Fricherry", "Satsuma",
};

static const char *const DAY_PLAIN[7] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday",
};

static const GColor DAY_COLORS[7] = {
    GColorFromRGB(170, 0, 0),
    GColorFromRGB(140, 70, 0),
    GColorFromRGB(0, 68, 170),
    GColorFromRGB(0, 145, 0),
    GColorFromRGB(160, 160, 0),
    GColorFromRGB(200, 0, 80),
    GColorFromRGB(255, 120, 0),
};

static const GRect ZONES[ANIM_ELEM_COUNT] = {
    [ANIM_ELEM_BAR] = GRect(0, 0, SCREEN_W, CAP_H + 60),
#if SOY_ROUND
    [ANIM_ELEM_TITLE] = GRect(0, DAY_Y - 5, SCREEN_W, 36),
  #ifdef FLAVOR_CENTERED
    [ANIM_ELEM_FLAVOR] = GRect(0, 126, SCREEN_W, 28),
  #else
    [ANIM_ELEM_FLAVOR] = GRect(0, FLAVOR_Y - 6, SCREEN_W, 26),
  #endif
  #ifdef SOY_NO_BOX
    [ANIM_ELEM_BOX] = GRect(0, 0, 0, 0),
  #else
    [ANIM_ELEM_BOX] = GRect(0, BRAND_BOX_Y - 8, SCREEN_W,
                            BRAND_BOX_H_NORMAL + 18),
  #endif
  #if FOOTER_STYLE == FOOTER_ARC
    [ANIM_ELEM_FOOTER] = GRect(0, 0, SCREEN_W, CAP_H + 24),
  #else
    [ANIM_ELEM_FOOTER] = GRect(0, 0, 0, 0),
  #endif
#else
    [ANIM_ELEM_TITLE] = GRect(0, DAY_Y - 5, SCREEN_W, 36),
    [ANIM_ELEM_FLAVOR] = GRect(0, FLAVOR_Y - 6, SCREEN_W, 26),
  #ifndef SOY_NO_BOX
    [ANIM_ELEM_BOX] = GRect(0, BRAND_BOX_Y - 8, SCREEN_W,
                            BRAND_BOX_H_NORMAL + 18),
  #else
    [ANIM_ELEM_BOX] = GRect(0, 0, 0, 0),
  #endif
  #if FOOTER_STYLE == FOOTER_LINE
    [ANIM_ELEM_FOOTER] = GRect(0, FOOTER_Y - 8, SCREEN_W, 25),
  #else
    [ANIM_ELEM_FOOTER] = GRect(0, 0, 0, 0),
  #endif
#endif
    [ANIM_ELEM_CHIP_A] = GRect(0, CHIP_A_B.origin.y - 10, SCREEN_W,
                               SCREEN_H - CHIP_A_B.origin.y + 10),
    [ANIM_ELEM_CHIP_B] = GRect(0, CHIP_B_B.origin.y - 10, SCREEN_W,
                               SCREEN_H - CHIP_B_B.origin.y + 10),
};

static void draw_box(GContext *ctx, GRect r, int stroke_w) {
  (void)ctx;
  if (r.size.w == 0 || r.size.h == 0) {
    return;
  }
  if (stroke_w <= 0) {
    ta_fill_rect(r, GColorBlack);
    return;
  }
  ta_fill_rect(r, GColorWhite);
  const GRect top = GRect(r.origin.x, r.origin.y, r.size.w, stroke_w);
  const GRect bot =
      GRect(r.origin.x, r.origin.y + r.size.h - stroke_w, r.size.w, stroke_w);
  const GRect left = GRect(r.origin.x, r.origin.y, stroke_w, r.size.h);
  const GRect right =
      GRect(r.origin.x + r.size.w - stroke_w, r.origin.y, stroke_w, r.size.h);
  ta_fill_rect(top, GColorBlack);
  ta_fill_rect(bot, GColorBlack);
  ta_fill_rect(left, GColorBlack);
  ta_fill_rect(right, GColorBlack);
}

static GRect offset_rect(GRect r, AnimElem e) {
  return GRect(r.origin.x + soy_anim_dx(e), r.origin.y + soy_anim_dy(e),
               r.size.w, r.size.h);
}

static bool begin_elem(AnimElem e) {
  if (!soy_anim_started(e)) {
    return false;
  }
  ta_set_clip(&ZONES[e]);
  return true;
}

static void end_elem(void) { ta_set_clip(NULL); }

static void render(Layer *layer, GContext *ctx) {
  (void)layer;
  const bool big = soy_state.big_time;
  char hour_buf[8];
  char min_buf[4];
  char line_buf[WEATHER_MAX_LEN + 10];
  const GColor accent = DAY_COLORS[soy_state.weekday];

  if (!ta_begin_frame(ctx)) {
      return;
  }

  ta_set_clip(NULL);
#if SOY_ROUND
  ta_fill_rect(GRect(0, 0, SCREEN_W, SCREEN_H), GColorBlack);
  ta_fill_circle_band(0, SCREEN_H, GColorWhite);
#else
  ta_fill_rect(GRect(0, 0, SCREEN_W, SCREEN_H), GColorWhite);
#endif

  if (begin_elem(ANIM_ELEM_BAR)) {
    const int16_t dy = soy_anim_dy(ANIM_ELEM_BAR);
#if SOY_ROUND
    ta_fill_circle_band(dy - 12, dy + CAP_H, accent);
#else
    ta_fill_rect(GRect(0, dy - 12, SCREEN_W, BAR_H + 12), accent);
#endif
    end_elem();
  }

  if (begin_elem(ANIM_ELEM_TITLE)) {
    ta_draw(ctx, &g_atlas_day24,
            soy_state.fruit_puns ? DAY_PUNS[soy_state.weekday]
                                 : DAY_PLAIN[soy_state.weekday],
            DAY_X + soy_anim_dx(ANIM_ELEM_TITLE),
            DAY_Y + soy_anim_dy(ANIM_ELEM_TITLE), accent, GColorWhite,
            SCREEN_W);
    end_elem();
  }

  if (soy_state.show_info && soy_state.weather[0] &&
      begin_elem(ANIM_ELEM_FLAVOR)) {
    snprintf(line_buf, sizeof(line_buf), "%s Flavored", soy_state.weather);
#ifdef FLAVOR_CENTERED
    ta_center(ctx, &FLAVOR_ATLAS, NULL, 0, line_buf, FLAVOR_BOX, GColorBlack,
              GColorWhite);
#else
    ta_draw(ctx, &FLAVOR_ATLAS, line_buf,
            FLAVOR_X + soy_anim_dx(ANIM_ELEM_FLAVOR),
            FLAVOR_Y + soy_anim_dy(ANIM_ELEM_FLAVOR), GColorBlack,
            GColorWhite, SCREEN_W);
#endif
    end_elem();
  }

#if !defined(SOY_NO_BOX)
  const int16_t box_h = big ? BRAND_BOX_H_BIG : BRAND_BOX_H_NORMAL;
  const int16_t stroke = big ? BRAND_STROKE_BIG : BRAND_STROKE_NORMAL;

  if (begin_elem(ANIM_ELEM_BOX)) {
    const GRect brand_box =
        offset_rect(GRect(BRAND_BOX_X, BRAND_BOX_Y, BRAND_BOX_W, box_h),
                    ANIM_ELEM_BOX);
    draw_box(ctx, brand_box, stroke);
    const int16_t inner_w = BRAND_BOX_W - 2 * stroke;

    TAMetrics m;
    ta_measure(&BRAND_MAIN, &BRAND_E, 'e', soy_state.brand, &m);
    if (m.width <= inner_w) {
      ta_center(ctx, &BRAND_MAIN, &BRAND_E, 'e',
                soy_state.brand,
                GRect(brand_box.origin.x + stroke,
                      brand_box.origin.y + stroke, inner_w,
                      box_h - 2 * stroke),
                GColorBlack, GColorWhite);
    } else {
      ta_center(ctx, &BRAND_FALLBACK, &BRAND_E_FALLBACK, 'e',
                soy_state.brand,
                GRect(brand_box.origin.x + stroke,
                      brand_box.origin.y + stroke, inner_w,
                      box_h - 2 * stroke),
                GColorBlack, GColorWhite);
    }
    end_elem();
  }
#endif

  if (soy_state.time_24h) {
    snprintf(hour_buf, sizeof(hour_buf), "%dg", soy_state.hours);
  } else {
    int h12 = soy_state.hours % 12;
    if (h12 == 0) {
      h12 = 12;
    }
    snprintf(hour_buf, sizeof(hour_buf), "%dg", h12);
  }
  snprintf(min_buf, sizeof(min_buf), "%02d", soy_state.minutes);
  const int16_t roll_h = big ? CHIP_B_B.size.h : MINUTE_NOMINAL_N.size.h;
  const bool rolling = soy_anim_minute_active();
  int16_t new_dy = 0, old_dy = 0;
  if (rolling) {
    soy_anim_roll_offsets(roll_h, &new_dy, &old_dy);
  }

  if (!big) {
    if (begin_elem(ANIM_ELEM_CHIP_A)) {
      const GRect chip_a = offset_rect(CHIP_A_N, ANIM_ELEM_CHIP_A);
      draw_box(ctx, chip_a, 0);
      ta_center(ctx, &CHIP_VAL_NORMAL, NULL, 0, hour_buf,
                GRect(chip_a.origin.x + 1, chip_a.origin.y + 1,
                      chip_a.size.w - 2, chip_a.size.h - 2),
                GColorWhite, GColorBlack);
#if defined(SOY_HAS_CHIP_LABELS)
      ta_draw_static_line(
          ctx, &g_line_label_protein,
          LABEL_PROTEIN_POS.x + soy_anim_dx(ANIM_ELEM_CHIP_A),
          LABEL_PROTEIN_POS.y + soy_anim_dy(ANIM_ELEM_CHIP_A), GColorBlack,
          GColorWhite);
#endif
      end_elem();
    }

    if (begin_elem(ANIM_ELEM_CHIP_B)) {
      const GRect min_box = offset_rect(MINUTE_NOMINAL_N, ANIM_ELEM_CHIP_B);
#if defined(SOY_HAS_CHIP_LABELS)
      ta_draw_static_line(
          ctx, &g_line_label_minutes,
          LABEL_MINUTES_POS.x + soy_anim_dx(ANIM_ELEM_CHIP_B),
          LABEL_MINUTES_POS.y + soy_anim_dy(ANIM_ELEM_CHIP_B), GColorBlack,
          GColorWhite);
#endif
      if (rolling) {
        ta_set_clip(&min_box);
        ta_center(ctx, &CHIP_VAL_NORMAL, NULL, 0, min_buf,
                  GRect(min_box.origin.x, min_box.origin.y + new_dy,
                        min_box.size.w, min_box.size.h),
                  GColorBlack, GColorWhite);
        ta_center(ctx, &CHIP_VAL_NORMAL, NULL, 0, soy_anim_minute_old(),
                  GRect(min_box.origin.x, min_box.origin.y + old_dy,
                        min_box.size.w, min_box.size.h),
                  GColorBlack, GColorWhite);
      } else {
        ta_center(ctx, &CHIP_VAL_NORMAL, NULL, 0, min_buf, min_box,
                  GColorBlack, GColorWhite);
      }
      end_elem();
    }
  } else {
    if (begin_elem(ANIM_ELEM_CHIP_A)) {
      const GRect chip_a = offset_rect(CHIP_A_B, ANIM_ELEM_CHIP_A);
      draw_box(ctx, chip_a, 0);
      ta_center(ctx, &g_atlas_val36, NULL, 0, hour_buf,
                GRect(chip_a.origin.x + 2, chip_a.origin.y + 2,
                      chip_a.size.w - 4, chip_a.size.h - 4),
                GColorWhite, GColorBlack);
      end_elem();
    }

    if (begin_elem(ANIM_ELEM_CHIP_B)) {
      const GRect chip_b = offset_rect(CHIP_B_B, ANIM_ELEM_CHIP_B);
      draw_box(ctx, chip_b, 1);
      if (rolling) {
        const GRect mask = GRect(chip_b.origin.x + 2, chip_b.origin.y + 2,
                                 chip_b.size.w - 4, chip_b.size.h - 4);
        ta_set_clip(&mask);
        ta_center(ctx, &g_atlas_val36, NULL, 0, min_buf,
                  GRect(chip_b.origin.x, chip_b.origin.y + new_dy,
                        chip_b.size.w, chip_b.size.h),
                  GColorBlack, GColorWhite);
        ta_center(ctx, &g_atlas_val36, NULL, 0, soy_anim_minute_old(),
                  GRect(chip_b.origin.x, chip_b.origin.y + old_dy,
                        chip_b.size.w, chip_b.size.h),
                  GColorBlack, GColorWhite);
      } else {
        ta_center(ctx, &g_atlas_val36, NULL, 0, min_buf, chip_b,
                  GColorBlack, GColorWhite);
      }
      end_elem();
    }
  }

#if FOOTER_STYLE == FOOTER_LINE
  if (soy_state.show_info && begin_elem(ANIM_ELEM_FOOTER)) {
    char tbuf[5];
    char pct[4];
    snprintf(tbuf, sizeof(tbuf), "%d", soy_state.temp);
    snprintf(pct, sizeof(pct), "%d", soy_state.battery);
    TAMetrics tm_, bm;
    ta_measure(&g_atlas_val12, NULL, 0, tbuf, &tm_);
    ta_measure(&g_atlas_val12, NULL, 0, pct, &bm);
    const int16_t gap_deg = 1;
    const int16_t gap_ml = 2;
    const int16_t total_ink = tm_.ink_w + gap_deg + g_line_footer_left.w +
                              bm.ink_w + gap_ml + g_line_footer_right.w;
    const int16_t fx =
        (SCREEN_W - total_ink) / 2 + soy_anim_dx(ANIM_ELEM_FOOTER);
    const int16_t fy = FOOTER_Y + soy_anim_dy(ANIM_ELEM_FOOTER);
    const int16_t deg_x = fx + tm_.ink_w + gap_deg;
    const int16_t batt_pen = deg_x + g_line_footer_left.w - bm.ink_left;
    const int16_t ml_x = batt_pen + bm.ink_left + bm.ink_w + gap_ml;
    ta_draw(ctx, &g_atlas_val12, tbuf, fx - tm_.ink_left, fy, GColorBlack,
            GColorWhite, -1);
    ta_draw_static_line(ctx, &g_line_footer_left, deg_x, fy, GColorBlack,
                        GColorWhite);
    ta_draw(ctx, &g_atlas_val12, pct, batt_pen, fy, GColorBlack, GColorWhite,
            -1);
    ta_draw_static_line(ctx, &g_line_footer_right, ml_x, fy, GColorBlack,
                        GColorWhite);
    end_elem();
  }
#elif FOOTER_STYLE == FOOTER_ARC
  if (soy_state.show_info && begin_elem(ANIM_ELEM_FOOTER)) {
    char tbuf[5];
    char pct[4];
    snprintf(tbuf, sizeof(tbuf), "%d", soy_state.temp);
    snprintf(pct, sizeof(pct), "%d", soy_state.battery);
    const TaArcPiece pieces[4] = {
        {.atlas = &g_atlas_val12, .text = tbuf, .gap_before = 0},
        {.line = &g_line_footer_left, .gap_before = 1},
        {.atlas = &g_atlas_val12, .text = pct, .gap_before = 2},
        {.line = &g_line_footer_right, .gap_before = 0},
    };
    ta_draw_arc(ctx, pieces, 4, ARC_CX,
                ARC_CY + soy_anim_dy(ANIM_ELEM_FOOTER), ARC_R, GColorWhite,
                accent);
    end_elem();
  }
#endif

  ta_set_clip(NULL);
  ta_end_frame(ctx);
}

void ui_refresh(void) {
  if (s_window && window_is_loaded(s_window)) {
    layer_mark_dirty(window_get_root_layer(s_window));
  }
}

void ui_init(Window *window) {
  s_window = window;
#if SOY_ROUND
  window_set_background_color(window, GColorBlack);
#else
  window_set_background_color(window, GColorWhite);
#endif
  layer_set_update_proc(window_get_root_layer(window), render);
}

void ui_deinit(Window *window) {
  (void)window;
  s_window = NULL;
}
