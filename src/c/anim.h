#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  ANIM_ELEM_BAR = 0,
  ANIM_ELEM_TITLE,
  ANIM_ELEM_FLAVOR,
  ANIM_ELEM_BOX,
  ANIM_ELEM_CHIP_A,
  ANIM_ELEM_CHIP_B,
  ANIM_ELEM_FOOTER,
  ANIM_ELEM_COUNT
} AnimElem;

void soy_anim_init(void);
void soy_anim_deinit(void);
void soy_anim_boot(void);
void soy_anim_set_enabled(bool enabled);

void soy_anim_minute_trigger(const char *old_value);
bool soy_anim_minute_active(void);
const char *soy_anim_minute_old(void);

void soy_anim_roll_offsets(int16_t h, int16_t *new_dy, int16_t *old_dy);

bool soy_anim_active(void);

int16_t soy_anim_dx(AnimElem e);
int16_t soy_anim_dy(AnimElem e);

bool soy_anim_started(AnimElem e);

void soy_anim_minute_trigger(const char *old_value);
bool soy_anim_minute_active(void);
const char *soy_anim_minute_old(void);

void soy_anim_roll_offsets(int16_t h, int16_t *new_dy, int16_t *old_dy);
