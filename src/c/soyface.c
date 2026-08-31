#include <pebble.h>
#include <stdio.h>
#include <string.h>

#include "anim.h"
#include "keys.h"
#include "ui.h"

static Window *s_window;

static void sync_clock(void) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  soy_state.hours = t->tm_hour;
  soy_state.minutes = t->tm_min;
  soy_state.weekday = t->tm_wday;
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & MINUTE_UNIT) {
    char old_min[4];
    snprintf(old_min, sizeof(old_min), "%02d", soy_state.minutes);
    soy_anim_minute_trigger(old_min);
  }
  soy_state.hours = tick_time->tm_hour;
  soy_state.minutes = tick_time->tm_min;
  soy_state.weekday = tick_time->tm_wday;
  ui_refresh();
}

static void battery_handler(BatteryChargeState state) {
  soy_state.battery = (uint8_t)state.charge_percent;
  ui_refresh();
}

static void store_brand(const char *src) {
  strncpy(soy_state.brand, src, BRAND_MAX_LEN);
  soy_state.brand[BRAND_MAX_LEN] = '\0';
  persist_write_string(PKEY_BRAND, soy_state.brand);
  ui_refresh();
}

static void store_weather(const char *src) {
  strncpy(soy_state.weather, src, WEATHER_MAX_LEN);
  soy_state.weather[WEATHER_MAX_LEN] = '\0';
  persist_write_string(PKEY_WEATHER, soy_state.weather);
  ui_refresh();
}

static void store_temp(int8_t temp) {
  soy_state.temp = temp;
  persist_write_int(PKEY_TEMP, temp);
  ui_refresh();
}

static void store_show_info(bool show) {
  soy_state.show_info = show;
  persist_write_bool(PKEY_INFO, show);
  ui_refresh();
}

static void store_time_24h(bool v) {
  soy_state.time_24h = v;
  persist_write_bool(PKEY_TIME24, v);
  ui_refresh();
}

static void store_animations(bool v) {
  soy_state.animations = v;
  soy_anim_set_enabled(v);
  persist_write_bool(PKEY_ANIM, v);
  ui_refresh();
}

static void store_fruit_puns(bool v) {
  soy_state.fruit_puns = v;
  persist_write_bool(PKEY_PUNS, v);
  ui_refresh();
}

static void store_temp_unit(bool celsius) {
  soy_state.temp_celsius = celsius;
  persist_write_bool(PKEY_TEMP_UNIT, celsius);
  ui_refresh();
}

static void store_buzz(bool v) {
  soy_state.buzz = v;
  persist_write_bool(PKEY_BUZZ, v);
}

static void bluetooth_handler(bool connected) {
  if (!connected && soy_state.buzz) {
    vibes_long_pulse();
  }
}

static void store_big_time(bool big) {
  soy_state.big_time = big;
  persist_write_int(PKEY_MODE, big ? 1 : 0);
  ui_refresh();
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  (void)context;
  Tuple *brand = dict_find(iter, KEY_BRAND_TEXT);
  if (brand && brand->type == TUPLE_CSTRING && brand->value->cstring[0]) {
    store_brand(brand->value->cstring);
  }
  Tuple *weather = dict_find(iter, KEY_WEATHER_LABEL);
  if (weather && weather->type == TUPLE_CSTRING &&
      weather->value->cstring[0]) {
    store_weather(weather->value->cstring);
  }
  Tuple *temp = dict_find(iter, KEY_WEATHER_TEMP);
  if (temp && (temp->type == TUPLE_INT || temp->type == TUPLE_UINT)) {
    store_temp((int8_t)temp->value->int32);
  }
  Tuple *info = dict_find(iter, KEY_SHOW_INFO);
  if (info && info->type == TUPLE_INT) {
    store_show_info(info->value->int32 != 0);
  }
  Tuple *big = dict_find(iter, KEY_BIG_TIME);
  if (big && big->type == TUPLE_INT) {
    store_big_time(big->value->int32 != 0);
  }
  Tuple *t24 = dict_find(iter, KEY_TIME_24H);
  if (t24 && t24->type == TUPLE_INT) {
    store_time_24h(t24->value->int32 != 0);
  }
  Tuple *anim = dict_find(iter, KEY_ANIMATIONS);
  if (anim && anim->type == TUPLE_INT) {
    store_animations(anim->value->int32 != 0);
  }
  Tuple *puns = dict_find(iter, KEY_FRUIT_PUNS);
  if (puns && puns->type == TUPLE_INT) {
    store_fruit_puns(puns->value->int32 != 0);
  }
  Tuple *tunit = dict_find(iter, KEY_TEMP_UNIT);
  if (tunit && tunit->type == TUPLE_INT) {
    store_temp_unit(tunit->value->int32 != 0);
  }
  Tuple *buzz = dict_find(iter, KEY_BUZZ);
  if (buzz && buzz->type == TUPLE_INT) {
    store_buzz(buzz->value->int32 != 0);
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  (void)context;
  APP_LOG(APP_LOG_LEVEL_WARNING, "message dropped: %d", (int)reason);
}

static void window_load(Window *window) {
  ui_init(window);
}

static void window_appear(Window *window) {
  soy_anim_boot();
}

static void window_unload(Window *window) {
  ui_deinit(window);
}

static void unobstructed_will_change(GRect final_unobstructed_screen_area,
                                      void *context) {
  (void)context;
  (void)final_unobstructed_screen_area;
}

static void unobstructed_change(AnimationProgress progress, void *context) {
  (void)progress;
  (void)context;
  ui_refresh();
}

static void unobstructed_did_change(void *context) {
  (void)context;
  ui_refresh();
}

static void init(void) {
  strcpy(soy_state.brand, BRAND_DEFAULT);
  soy_state.big_time = false;
  strcpy(soy_state.weather, WEATHER_DEFAULT);
  soy_state.big_time = false;
  soy_state.show_info = true;
  soy_state.time_24h = false;
  soy_state.animations = true;
  soy_state.fruit_puns = true;
  soy_state.temp_celsius = false;
  soy_state.buzz = false;
  soy_state.battery = 100;
  soy_state.temp = TEMP_DEFAULT;

  if (persist_exists(PKEY_WEATHER)) {
    persist_read_string(PKEY_WEATHER, soy_state.weather,
                        sizeof(soy_state.weather));
  }
  if (persist_exists(PKEY_BRAND)) {
    persist_read_string(PKEY_BRAND, soy_state.brand, sizeof(soy_state.brand));
  }
  if (persist_exists(PKEY_MODE)) {
    soy_state.big_time = persist_read_int(PKEY_MODE) != 0;
  }
  if (persist_exists(PKEY_INFO)) {
    soy_state.show_info = persist_read_bool(PKEY_INFO);
  }
  if (persist_exists(PKEY_TEMP)) {
    soy_state.temp = (int8_t)persist_read_int(PKEY_TEMP);
  }
  if (persist_exists(PKEY_TIME24)) {
    soy_state.time_24h = persist_read_bool(PKEY_TIME24);
  }
  if (persist_exists(PKEY_ANIM)) {
    soy_state.animations = persist_read_bool(PKEY_ANIM);
  }
  if (persist_exists(PKEY_PUNS)) {
    soy_state.fruit_puns = persist_read_bool(PKEY_PUNS);
  }
  if (persist_exists(PKEY_TEMP_UNIT)) {
    soy_state.temp_celsius = persist_read_bool(PKEY_TEMP_UNIT);
  }
  if (persist_exists(PKEY_BUZZ)) {
    soy_state.buzz = persist_read_bool(PKEY_BUZZ);
  }
  sync_clock();
  soy_state.battery =
      (uint8_t)battery_state_service_peek().charge_percent;

  soy_anim_set_enabled(soy_state.animations);
  soy_anim_init();

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = window_load,
                                           .appear = window_appear,
                                           .unload = window_unload,
                                       });
  window_stack_push(s_window, false);

  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
#if defined(PBL_PLATFORM_CHALK) || defined(PBL_PLATFORM_BASALT) || \
    defined(PBL_PLATFORM_APLITE) || defined(PBL_PLATFORM_DIORITE)
  app_message_open(512, 64);
#else
  app_message_open(app_message_inbox_size_maximum(),
                   app_message_outbox_size_maximum());
#endif

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
  unobstructed_area_service_subscribe((UnobstructedAreaHandlers){
                                          .will_change = unobstructed_will_change,
                                          .change = unobstructed_change,
                                          .did_change = unobstructed_did_change,
                                      },
                                      NULL);
#if defined(PBL_PLATFORM_GABBRO)
  connection_service_subscribe((ConnectionHandlers){
      .pebble_app_connection_handler = bluetooth_handler,
      .pebblekit_connection_handler = NULL,
  });
#else
  bluetooth_connection_service_subscribe(bluetooth_handler);
#endif
}

static void deinit(void) {
  unobstructed_area_service_unsubscribe();
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  app_message_deregister_callbacks();
  soy_anim_deinit();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
