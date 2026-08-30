#pragma once

#include <pebble.h>

#define BRAND_MAX_LEN 15
#define WEATHER_MAX_LEN 13

typedef struct {
  bool big_time;
  bool show_info;
  bool time_24h;
  bool animations;
  bool fruit_puns;
  bool temp_celsius;
  bool buzz;
  uint8_t hours;
  uint8_t minutes;
  uint8_t weekday;
  uint8_t battery;
  int8_t temp;
  char brand[BRAND_MAX_LEN + 1];
  char weather[WEATHER_MAX_LEN + 1];
} SoyState;

extern SoyState soy_state;

void ui_init(Window *window);
void ui_deinit(Window *window);
void ui_refresh(void);
