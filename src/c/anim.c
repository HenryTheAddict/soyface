#include "anim.h"

#include <pebble.h>

#include "ui.h"

#define SPRING_K 170.0f
#define SPRING_C 16.0f
#define SETTLE_POS 0.4f
#define SETTLE_VEL 6.0f
#define MAX_DT_S 0.05f
#define FRAME_MS 16
#define ROLL_NOMINAL_H 64
#define ROLL_TIMEOUT_MS 2500

typedef struct {
  float d;
  float v;
  int64_t start_ms;
} Spring;

static Spring s_elem[ANIM_ELEM_COUNT];

static Spring s_roll = {.d = 0, .v = 0, .start_ms = 0};
static bool s_roll_on;
static char s_old_min[4];

static AppTimer *s_timer;
static uint64_t s_last_frame_us;
static bool s_booted;
static bool s_enabled = true;

typedef struct {
  int16_t dx0, dy0;
  uint16_t delay_ms;
} Choreo_t;

static const Choreo_t CHOREO[ANIM_ELEM_COUNT] = {
    [ANIM_ELEM_BAR] = {.dx0 = 0, .dy0 = -32, .delay_ms = 0},
    [ANIM_ELEM_TITLE] = {.dx0 = -60, .dy0 = 0, .delay_ms = 80},
    [ANIM_ELEM_FLAVOR] = {.dx0 = -48, .dy0 = 0, .delay_ms = 150},
    [ANIM_ELEM_BOX] = {.dx0 = 0, .dy0 = 30, .delay_ms = 220},
    [ANIM_ELEM_CHIP_A] = {.dx0 = -40, .dy0 = 0, .delay_ms = 290},
    [ANIM_ELEM_CHIP_B] = {.dx0 = 40, .dy0 = 0, .delay_ms = 350},
    [ANIM_ELEM_FOOTER] = {.dx0 = 0, .dy0 = 22, .delay_ms = 410},
};

static int64_t now_ms(void) {
  time_t t = time(NULL);
  uint16_t ms;
  time_ms(&t, &ms);
  return (int64_t)t * 1000 + ms;
}

static bool start_reached(const Spring *s) {
  const int64_t now = now_ms();
  if (now >= s->start_ms) {
    return true;
  }
  return (s->start_ms - now) > 10000;
}

static void spring_kick(Spring *s, float d0, int64_t delay_ms) {
  s->d = d0;
  s->v = 0;
  s->start_ms = now_ms() + delay_ms;
}

static bool spring_step(Spring *s, float dt_s) {
  if (!start_reached(s)) {
    return true;
  }
  const float a = -SPRING_K * s->d - SPRING_C * s->v;
  s->v += a * dt_s;
  s->d += s->v * dt_s;
  return !(s->d > -SETTLE_POS && s->d < SETTLE_POS && s->v > -SETTLE_VEL &&
           s->v < SETTLE_VEL);
}

static void frame_cb(void *data) {
  (void)data;
  const uint64_t now_us = now_ms() * 1000;
  float dt = (s_last_frame_us == 0)
                 ? 0.033f
                 : (float)(now_us - s_last_frame_us) / 1000000.0f;
  s_last_frame_us = now_us;
  if (dt <= 0 || dt > MAX_DT_S) {
    dt = MAX_DT_S;
  }

  bool moving = false;
  for (int i = 0; i < ANIM_ELEM_COUNT; i++) {
    moving |= spring_step(&s_elem[i], dt);
  }

  if (s_roll_on) {
    moving |= spring_step(&s_roll, dt);
    if (!moving || now_ms() - s_roll.start_ms > ROLL_TIMEOUT_MS) {
      s_roll_on = false;
      s_roll.d = 0;
      s_roll.v = 0;
      moving = false;
    }
  }

  ui_refresh();

  if (moving) {
    s_timer = app_timer_register(FRAME_MS, frame_cb, NULL);
  } else {
    s_timer = NULL;
    s_last_frame_us = 0;
  }
}

static void driver_ensure_running(void) {
  if (!s_timer) {
    s_last_frame_us = 0;
    s_timer = app_timer_register(FRAME_MS, frame_cb, NULL);
  }
}

void soy_anim_init(void) {
}

void soy_anim_deinit(void) {
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
}

void soy_anim_set_enabled(bool enabled) {
  s_enabled = enabled;
}

void soy_anim_boot(void) {
  if (!s_enabled) {
    s_booted = false;
    return;
  }
  const int64_t base = now_ms();
  for (int i = 0; i < ANIM_ELEM_COUNT; i++) {
    spring_kick(&s_elem[i],
                CHOREO[i].dx0 ? (float)CHOREO[i].dx0 : (float)CHOREO[i].dy0,
                CHOREO[i].delay_ms);
  }
  s_booted = true;
  driver_ensure_running();
}

bool soy_anim_active(void) { return s_timer != NULL; }

bool soy_anim_started(AnimElem e) {
  return !s_booted || start_reached(&s_elem[e]);
}

int16_t soy_anim_dx(AnimElem e) {
  return CHOREO[e].dx0 ? (int16_t)s_elem[e].d : 0;
}

int16_t soy_anim_dy(AnimElem e) {
  return (CHOREO[e].dy0 && !CHOREO[e].dx0) ? (int16_t)s_elem[e].d : 0;
}

void soy_anim_minute_trigger(const char *old_value) {
  if (!s_enabled) {
    return;
  }
  s_old_min[0] = '\0';
  if (old_value) {
    strncpy(s_old_min, old_value, sizeof(s_old_min) - 1);
    s_old_min[sizeof(s_old_min) - 1] = '\0';
  }
  spring_kick(&s_roll, (float)ROLL_NOMINAL_H, 0);
  s_roll_on = true;
  driver_ensure_running();
}

bool soy_anim_minute_active(void) {
  if (!s_roll_on) {
    return false;
  }
  if (now_ms() - s_roll.start_ms > ROLL_TIMEOUT_MS) {
    s_roll_on = false;
    s_roll.d = 0;
    s_roll.v = 0;
    return false;
  }
  return true;
}

const char *soy_anim_minute_old(void) { return s_old_min; }

void soy_anim_roll_offsets(int16_t h, int16_t *new_dy, int16_t *old_dy) {
  const float scale = (float)h / ROLL_NOMINAL_H;
  *new_dy = (int16_t)(s_roll.d * scale + 0.5f);
  *old_dy = *new_dy - h;
}
