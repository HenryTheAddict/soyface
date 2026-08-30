#pragma once

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((packed)) {
    uint16_t offset;
    uint8_t w, h;
    int8_t left;
    int8_t top;
    uint8_t advance;
} Glyph;

typedef struct {
    const uint8_t *data;
    const Glyph *glyphs;
    const uint8_t *map;
    uint8_t ascent;
    uint8_t glyph_count;
} Atlas;

typedef struct {
    const uint8_t *data;
    uint8_t w, h;
} StaticLine;

extern const Atlas g_atlas_brand32;
#define GLYPH_COUNT_brand32 95
extern const Atlas g_atlas_brand_e32;
#define GLYPH_COUNT_brand_e32 1
extern const Atlas g_atlas_brand48;
#define GLYPH_COUNT_brand48 95
extern const Atlas g_atlas_brand_e48;
#define GLYPH_COUNT_brand_e48 1
extern const Atlas g_atlas_brand40;
#define GLYPH_COUNT_brand40 95
extern const Atlas g_atlas_brand_e40;
#define GLYPH_COUNT_brand_e40 1
extern const Atlas g_atlas_day24;
#define GLYPH_COUNT_day24 53
extern const Atlas g_atlas_flavor14;
#define GLYPH_COUNT_flavor14 53
extern const Atlas g_atlas_flavor12;
#define GLYPH_COUNT_flavor12 53
extern const Atlas g_atlas_rg9;
#define GLYPH_COUNT_rg9 53
extern const Atlas g_atlas_val20;
#define GLYPH_COUNT_val20 12
extern const Atlas g_atlas_val36;
#define GLYPH_COUNT_val36 12
extern const Atlas g_atlas_val12;
#define GLYPH_COUNT_val12 13
extern const StaticLine g_line_label_protein;
extern const StaticLine g_line_label_minutes;
extern const StaticLine g_line_footer_left;
extern const StaticLine g_line_footer_right;

#ifdef __cplusplus
}
#endif
