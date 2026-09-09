#include QMK_KEYBOARD_H
#include "version.h"
#define MOON_LED_LEVEL LED_LEVEL
#ifndef ZSA_SAFE_RANGE
#define ZSA_SAFE_RANGE SAFE_RANGE
#endif

enum custom_keycodes {
  RGB_SLD = ZSA_SAFE_RANGE,
  ST_MACRO_0,
  ST_MACRO_1,
  ST_MACRO_2,
  ST_MACRO_3,
  ST_MACRO_4,
  ST_MACRO_5,
  ST_MACRO_6,
  ST_MACRO_7,
  AUTOCLK_R,
  AUTOCLK_L,
  DUALHOLD,
  DUALTOG,
};


/* ==================================================================
 * AUTOCLK_R / AUTOCLK_L - hold-to-autoclick
 *
 * Holding either key repeatedly taps that instance's mouse button,
 * with a random gap of min_ms..max_ms between clicks. Releasing stops
 * immediately and leaves the button up. The two are independent and
 * can be held at the same time.
 *
 * The defines below are the shared defaults; to give one clicker
 * different timing, edit its initializer instead.
 * ================================================================== */

#define CLICK_MIN_MS   25  /* shortest gap between clicks (ms)   */
#define CLICK_MAX_MS   50  /* longest gap between clicks (ms)    */
#define CLICK_HOLD_MS  10  /* how long the button stays down (ms)*/
#define CLICK_MAX_HOLD 0   /* safety cutoff in ms; 0 disables it */

/* xorshift32 PRNG. Avoids rand()/srand(), which pull in more than we
 * need and behave differently across targets. Shared by both
 * clickers, which keeps their sequences from lining up. */
static uint32_t ac_rng = 0x2545F491UL;

static uint32_t ac_rng_next(void) {
  ac_rng ^= ac_rng << 13;
  ac_rng ^= ac_rng >> 17;
  ac_rng ^= ac_rng << 5;
  return ac_rng;
}

/* inclusive range [lo, hi] */
static uint8_t ac_rng_range(uint8_t lo, uint8_t hi) {
  return lo + (uint8_t)(ac_rng_next() % (uint32_t)(hi - lo + 1));
}

typedef struct {
  /* configuration */
  uint16_t button;    /* which mouse button this clicker taps */
  uint8_t  min_ms;
  uint8_t  max_ms;
  uint8_t  hold_ms;
  uint16_t max_hold;
  /* runtime state */
  bool     active;
  bool     btn_down;
  uint16_t last;
  uint16_t started;
  uint8_t  delay;
} autoclick_t;

static autoclick_t ac_right = {
  .button   = KC_MS_BTN2,
  .min_ms   = CLICK_MIN_MS,
  .max_ms   = CLICK_MAX_MS,
  .hold_ms  = CLICK_HOLD_MS,
  .max_hold = CLICK_MAX_HOLD,
};

static autoclick_t ac_left = {
  .button   = KC_MS_BTN1,
  .min_ms   = CLICK_MIN_MS,
  .max_ms   = CLICK_MAX_MS,
  .hold_ms  = CLICK_HOLD_MS,
  .max_hold = CLICK_MAX_HOLD,
};

static void autoclick_stop(autoclick_t *ac) {
  ac->active = false;
  if (ac->btn_down) {
    unregister_code(ac->button);
    ac->btn_down = false;
  }
}

static void autoclick_start(autoclick_t *ac) {
  /* reseed off the running timer so each hold differs */
  ac_rng ^= timer_read32();
  if (ac_rng == 0) {
    ac_rng = 0x2545F491UL;
  }
  ac->active   = true;
  ac->btn_down = false;
  ac->last     = timer_read();
  ac->started  = ac->last;
  ac->delay    = 0;               /* fire on the next matrix scan */
}

static void autoclick_task(autoclick_t *ac) {
  if (!ac->active) {
    return;
  }

  if (ac->max_hold > 0 && timer_elapsed(ac->started) > ac->max_hold) {
    autoclick_stop(ac);
    return;
  }

  if (timer_elapsed(ac->last) < ac->delay) {
    return;
  }

  if (ac->btn_down) {
    unregister_code(ac->button);
    ac->btn_down = false;
    ac->delay    = ac_rng_range(ac->min_ms, ac->max_ms);
  } else {
    register_code(ac->button);
    ac->btn_down = true;
    ac->delay    = ac->hold_ms;
  }

  ac->last = timer_read();
}


/* ==================================================================
 * DUALHOLD / DUALTOG - hold two letters down
 *
 * DUALHOLD is momentary: both DH_KEY_1 and DH_KEY_2 go down while the
 * key is held and release when it is let go.
 *
 * DUALTOG is a latch: one tap holds both letters, another tap
 * releases them.
 *
 * Both share dh_active, so DUALHOLD's release will also clear a latch
 * set by DUALTOG. That keeps the two from fighting and means neither
 * can leave a letter stuck down.
 *
 * Caveat (mainly affects DUALTOG): physically typing either letter
 * while the letters are held will send that key's release and
 * silently break it for that letter.
 * ================================================================== */

#define DH_KEY_1 KC_I
#define DH_KEY_2 KC_J

static bool dh_active = false;

static void dualhold_stop(void) {
  if (dh_active) {
    del_key(DH_KEY_1);
    del_key(DH_KEY_2);
    send_keyboard_report();
    dh_active = false;
  }
}

static void dualhold_start(void) {
  if (!dh_active) {
    add_key(DH_KEY_1);
    add_key(DH_KEY_2);
    send_keyboard_report();
    dh_active = true;
  }
}




const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [0] = LAYOUT_moonlander(
    KC_GRAVE,       KC_1,           KC_2,                KC_3,                 KC_4,               KC_5,           MO(4),                                          MO(7),          KC_6,           KC_7,                 KC_8,                KC_9,                KC_0,           KC_DELETE,
    KC_TAB,         KC_Q,           KC_W,                KC_E,                 KC_R,               KC_T,           ST_MACRO_0,                                     TG(9),          KC_Y,           KC_U,                 KC_I,                KC_O,                KC_P,           KC_BSLS,
    KC_ESCAPE,      KC_A,           KC_S,                KC_D,                 KC_F,               KC_G,           ST_MACRO_1,                                     DM_PLY1,        KC_H,           KC_J,                 KC_K,                KC_L,                KC_SCLN,        KC_QUOTE,
    KC_LEFT_SHIFT,  KC_Z,           KC_X,                KC_C,                 KC_V,               KC_B,                                                                           KC_N,           KC_M,                 KC_COMMA,            KC_DOT,              KC_SLASH,       KC_RIGHT_SHIFT,
    KC_APPLICATION, KC_RIGHT_ALT,   RALT(KC_RIGHT_CTRL), KC_LEFT_GUI,          KC_LEFT_ALT,        TG(8),                                                                          TG(1),          KC_LEFT,              KC_DOWN,             KC_UP,               KC_RIGHT,       KC_CAPS,
    KC_SPACE,       KC_BSPC,        KC_LEFT_CTRL,                                                                                                                                                                                             MO(2),               MO(3),          KC_ENTER
  ),
  [1] = LAYOUT_moonlander(
    KC_NO,          KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_NO,                                          KC_NO,          KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_NO,                                          KC_NO,          KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    TO(0),          KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_NO,                                          KC_NO,          KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT,                                                                 KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_NO,          KC_NO,          KC_NO,               KC_NO,                KC_NO,              KC_NO,                                                                          KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_UNDS,        KC_TRANSPARENT, KC_NO,                                                                                                                                                                                                    KC_TRANSPARENT,      KC_TRANSPARENT, TO(0)
  ),
  [2] = LAYOUT_moonlander(
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_NO,                                          KC_NO,          KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, RALT(KC_E),          RALT(KC_F),           RALT(KC_G),         KC_TRANSPARENT, KC_NO,                                          KC_NO,          KC_TRANSPARENT, RALT(KC_H),           RALT(KC_V),          RALT(KC_COMMA),      RALT(KC_DOT),   KC_TRANSPARENT,
    KC_TRANSPARENT, RALT(KC_Z),     ST_MACRO_2,          KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_NO,                                          KC_NO,          KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT,                                                                 KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_NO,          KC_NO,          KC_NO,               KC_NO,                KC_NO,              KC_TRANSPARENT,                                                                 KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_NO,                                                                                                                                                                                                    KC_TRANSPARENT,      KC_NO,          KC_TRANSPARENT
  ),
  [3] = LAYOUT_moonlander(
    KC_TRANSPARENT, KC_F1,          KC_F2,               KC_F3,                KC_F4,              KC_F5,          KC_NO,                                          KC_NO,          KC_F6,          KC_F7,                KC_F8,               KC_F9,               KC_F10,         KC_F11,
    KC_GRAVE,       KC_EXLM,        KC_AT,               KC_HASH,              KC_DLR,             KC_PERC,        KC_NO,                                          KC_NO,          KC_CIRC,        KC_AMPR,              KC_ASTR,             KC_MINUS,            KC_EQUAL,       KC_PIPE,
    KC_NO,          KC_LBRC,        KC_RBRC,             KC_LPRN,              KC_RPRN,            KC_NO,          KC_NO,                                          KC_NO,          ST_MACRO_3,     ST_MACRO_4,           KC_NO,               KC_UNDS,             KC_COLN,        KC_DQUO,
    KC_TRANSPARENT, LSFT(KC_GRAVE), RALT(KC_EQUAL),      KC_LCBR,              KC_RCBR,            KC_NO,                                                                          RALT(KC_MINUS), RALT(LSFT(KC_MINUS)), KC_LABK,             KC_RABK,             KC_QUES,        KC_TRANSPARENT,
    KC_NO,          KC_NO,          KC_NO,               KC_NO,                KC_NO,              KC_NO,                                                                          KC_MS_BTN3,     KC_HOME,              KC_NO,               KC_NO,               KC_END,         KC_F12,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,                                                                                                                                                                                           KC_NO,               KC_TRANSPARENT, KC_NO
  ),
  [4] = LAYOUT_moonlander(
    KC_TRANSPARENT, OSL(6),         KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_TRANSPARENT,                                 KC_NO,          KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, DM_REC1,                                        KC_NO,          KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_MY_COMPUTER,      KC_PSCR,        KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, DM_RSTP,                                        KC_NO,          KC_TRANSPARENT, KC_MEDIA_PREV_TRACK,  KC_MEDIA_PLAY_PAUSE, KC_MEDIA_NEXT_TRACK, KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT,                                                                 KC_TRANSPARENT, KC_AUDIO_MUTE,        KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT,                                                                 KC_TRANSPARENT, KC_TRANSPARENT,       KC_AUDIO_VOL_DOWN,   KC_AUDIO_VOL_UP,     KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,                                                                                                                                                                                           KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT
  ),
  [5] = LAYOUT_moonlander(
    ST_MACRO_5,     KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_NO,                                          KC_NO,          KC_NO,          KC_NO,                KC_NO,               KC_NO,               KC_NO,          KC_NO,
    KC_RCBR,        KC_COMMA,       KC_DOT,              LCTL(LSFT(KC_COMMA)), LCTL(LSFT(KC_DOT)), KC_NO,          KC_NO,                                          KC_NO,          KC_NO,          KC_NO,                KC_NO,               KC_NO,               KC_NO,          KC_NO,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       ST_MACRO_6,         KC_NO,          TO(0),                                          KC_NO,          KC_NO,          KC_NO,                KC_NO,               KC_NO,               KC_NO,          KC_NO,
    KC_NO,          LCTL(KC_Z),     KC_MS_WH_UP,         KC_MS_WH_DOWN,        LCTL(KC_V),         KC_NO,                                                                          KC_NO,          KC_NO,                KC_NO,               KC_NO,               KC_NO,          KC_NO,
    KC_NO,          KC_NO,          KC_NO,               KC_NO,                KC_NO,              KC_NO,                                                                          KC_NO,          KC_NO,                KC_NO,               KC_NO,               KC_NO,          KC_NO,
    KC_ENTER,       KC_TRANSPARENT, KC_NO,                                                                                                                                                                                                    KC_NO,               KC_NO,          KC_NO
  ),
  [6] = LAYOUT_moonlander(
    KC_TRANSPARENT, KC_TRANSPARENT, QK_BOOT,             KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_TRANSPARENT,                                 KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_TRANSPARENT,                                 KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_TRANSPARENT,                                 KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT,                                                                 KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT,                                                                 KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,                                                                                                                                                                                           KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT
  ),
  [7] = LAYOUT_moonlander(
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_TRANSPARENT,                                 KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_TRANSPARENT,                                 KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,       TO(5),               KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, ST_MACRO_7,          KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_TRANSPARENT,                                 KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT,                                                                 KC_NUM,         KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT,                                                                 KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, AS_TOGG,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,                                                                                                                                                                                           KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT
  ),
  [8] = LAYOUT_moonlander(
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_TRANSPARENT,                                 KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_TRANSPARENT,                                 KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_F,           KC_J,                KC_K,                 KC_L,               KC_F,           KC_TRANSPARENT,                                 KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_LABK,        KC_RABK,             KC_COMMA,             KC_DOT,             KC_TRANSPARENT,                                                                 KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT,                                                                 KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,                                                                                                                                                                                           KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT
  ),
  [9] = LAYOUT_moonlander(
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_TRANSPARENT,                                 KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_Y,                                           KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_H,                                           KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT,                                                                 KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     DUALTOG,                                                                        AUTOCLK_L,      KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    DUALHOLD,       AUTOCLK_R,      KC_LEFT_CTRL,                                                                                                                                                                                             KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT
  ),
  [10] = LAYOUT_moonlander(
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_TRANSPARENT,                                 KC_NO,          KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_Y,                                           KC_NO,          KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT, KC_H,                                           KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,     KC_TRANSPARENT,                                                                 KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_HYPR,        KC_MEH,         KC_RIGHT_ALT,        LSFT(KC_LEFT_ALT),    KC_LEFT_ALT,        KC_TRANSPARENT,                                                                 KC_TRANSPARENT, KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,                                                                                                                                                                                           KC_TRANSPARENT,      KC_TRANSPARENT, KC_TRANSPARENT
  ),
};


const uint16_t PROGMEM combo0[] = { KC_O, KC_I, KC_S, KC_D, KC_A, COMBO_END};
const uint16_t PROGMEM combo1[] = { KC_G, KC_R, KC_A, KC_M, COMBO_END};
const uint16_t PROGMEM combo2[] = { KC_S, KC_L, KC_A, KC_K, KC_C, COMBO_END};
const uint16_t PROGMEM combo3[] = { KC_T, KC_E, KC_S, KC_A, KC_M, COMBO_END};
const uint16_t PROGMEM combo4[] = { KC_F, KC_X, KC_I, KC_O, COMBO_END};
const uint16_t PROGMEM combo5[] = { KC_M, KC_A, KC_I, KC_L, COMBO_END};

combo_t key_combos[COMBO_COUNT] = {
    COMBO(combo0, LGUI(KC_4)),
    COMBO(combo1, LGUI(KC_5)),
    COMBO(combo2, LGUI(KC_6)),
    COMBO(combo3, LGUI(KC_7)),
    COMBO(combo4, LGUI(KC_1)),
    COMBO(combo5, LGUI(KC_8)),
};



extern rgb_config_t rgb_matrix_config;

RGB hsv_to_rgb_with_value(HSV hsv) {
  RGB rgb = hsv_to_rgb( hsv );
  float f = (float)rgb_matrix_config.hsv.v / UINT8_MAX;
  return (RGB){ f * rgb.r, f * rgb.g, f * rgb.b };
}

void keyboard_post_init_user(void) {
  rgb_matrix_enable();
}

const uint8_t PROGMEM ledmap[][RGB_MATRIX_LED_COUNT][3] = {
    [0] = { {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {0,0,0}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {0,199,255}, {68,213,205}, {68,213,205}, {217,179,255}, {217,179,255}, {217,179,255}, {0,0,0}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255}, {0,0,0}, {68,213,205}, {217,179,255}, {217,179,255}, {217,179,255}, {217,179,255} },

    [1] = { {0,0,0}, {129,255,255}, {217,179,255}, {129,255,255}, {0,0,0}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {0,0,0}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {0,0,0}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {0,0,0}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {0,0,0}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {0,0,0}, {0,0,0}, {0,0,0}, {172,255,255}, {129,255,255}, {0,0,0}, {0,0,0}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {129,255,255}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {129,255,255}, {129,255,255}, {217,179,255} },

    [2] = { {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {0,0,0}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {0,0,0}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {0,0,0}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {0,0,0}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {0,0,0}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {0,0,0}, {0,0,0}, {0,0,0}, {216,130,255}, {216,130,255}, {0,0,0}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {216,130,255}, {0,0,0}, {0,0,0}, {0,0,0}, {216,130,255}, {0,0,0}, {216,130,255}, {216,130,255} },

    [3] = { {175,228,186}, {175,228,186}, {0,0,0}, {175,228,186}, {0,0,0}, {64,255,205}, {175,228,186}, {175,228,186}, {175,228,186}, {0,0,0}, {64,255,205}, {175,228,186}, {175,228,186}, {175,228,186}, {0,0,0}, {64,255,205}, {175,228,186}, {175,228,186}, {175,228,186}, {0,0,0}, {64,255,205}, {175,228,186}, {175,228,186}, {175,228,186}, {175,228,186}, {64,255,205}, {175,228,186}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {175,228,186}, {175,228,186}, {175,228,186}, {0,0,0}, {64,255,205}, {175,228,186}, {175,228,186}, {175,228,186}, {64,255,205}, {64,255,205}, {175,228,186}, {175,228,186}, {175,228,186}, {0,0,0}, {64,255,205}, {175,228,186}, {175,228,186}, {175,228,186}, {0,0,0}, {64,255,205}, {175,228,186}, {0,0,0}, {175,228,186}, {0,0,0}, {64,255,205}, {175,228,186}, {175,228,186}, {175,228,186}, {0,0,0}, {64,255,205}, {175,228,186}, {175,228,186}, {175,228,186}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {175,228,186}, {0,0,0}, {0,0,0} },

    [4] = { {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,199,255}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {39,217,255}, {39,217,255}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {39,217,255}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {39,217,255}, {39,217,255}, {0,0,0}, {39,217,255}, {0,0,0}, {0,0,0}, {39,217,255}, {0,0,0}, {39,217,255}, {0,0,0}, {0,0,0}, {39,217,255}, {39,217,255}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0} },

    [5] = { {102,216,254}, {102,216,254}, {102,216,254}, {0,0,0}, {0,0,0}, {102,216,254}, {102,216,254}, {102,216,254}, {102,216,254}, {0,0,0}, {102,216,254}, {102,216,254}, {102,216,254}, {102,216,254}, {0,0,0}, {102,216,254}, {102,216,254}, {102,216,254}, {102,216,254}, {0,0,0}, {102,216,254}, {102,216,254}, {102,216,254}, {102,216,254}, {0,0,0}, {102,216,254}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {217,179,255}, {102,216,254}, {102,216,254}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0} },

    [6] = { {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,199,255}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0} },

    [7] = { {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {129,255,255}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {20,219,255}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {102,216,254}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0} },

    [10] = { {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {139,211,255}, {0,0,0}, {221,233,226}, {0,0,0}, {0,0,0}, {221,233,226}, {0,0,0}, {221,233,226}, {0,0,0}, {0,0,0}, {221,233,226}, {0,0,0}, {221,233,226}, {43,255,255}, {0,0,0}, {221,233,226}, {0,0,0}, {221,233,226}, {43,255,255}, {0,0,0}, {221,233,226}, {0,0,0}, {0,0,0}, {0,0,0}, {0,255,255}, {129,255,255}, {0,0,0}, {0,0,0}, {221,233,226}, {221,233,226}, {221,233,226}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {111,233,193}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {67,255,205}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {237,255,255}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {179,255,255}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0} },

};

void set_layer_color(int layer) {
  for (int i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
    HSV hsv = {
      .h = pgm_read_byte(&ledmap[layer][i][0]),
      .s = pgm_read_byte(&ledmap[layer][i][1]),
      .v = pgm_read_byte(&ledmap[layer][i][2]),
    };
    if (!hsv.h && !hsv.s && !hsv.v) {
        rgb_matrix_set_color( i, 0, 0, 0 );
    } else {
        RGB rgb = hsv_to_rgb_with_value(hsv);
        rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
    }
  }
}

bool rgb_matrix_indicators_user(void) {
  if (rawhid_state.rgb_control) {
      return false;
  }
  if (!keyboard_config.disable_layer_led) { 
    switch (biton32(layer_state)) {
      case 0:
        set_layer_color(0);
        break;
      case 1:
        set_layer_color(1);
        break;
      case 2:
        set_layer_color(2);
        break;
      case 3:
        set_layer_color(3);
        break;
      case 4:
        set_layer_color(4);
        break;
      case 5:
        set_layer_color(5);
        break;
      case 6:
        set_layer_color(6);
        break;
      case 7:
        set_layer_color(7);
        break;
      case 9: {
        /* Layer 9: flood every key with orange. Uses the same
         * brightness-scaling helper as set_layer_color, so it still
         * respects the user's RGB brightness setting. */
        HSV hsv = { HSV_ORANGE };
        RGB rgb = hsv_to_rgb_with_value(hsv);
        rgb_matrix_set_color_all(rgb.r, rgb.g, rgb.b);
        break;
      }
      case 10:
        set_layer_color(10);
        break;
     default:
        if (rgb_matrix_get_flags() == LED_FLAG_NONE) {
          rgb_matrix_set_color_all(0, 0, 0);
        }
    }
  } else {
    if (rgb_matrix_get_flags() == LED_FLAG_NONE) {
      rgb_matrix_set_color_all(0, 0, 0);
    }
  }

  return true;
}




bool process_record_user(uint16_t keycode, keyrecord_t *record) {
  switch (keycode) {
  case QK_MODS ... QK_MODS_MAX:
    // Mouse and consumer keys (volume, media) with modifiers work inconsistently across operating systems,
    // this makes sure that modifiers are always applied to the key that was pressed.
    if (IS_MOUSE_KEYCODE(QK_MODS_GET_BASIC_KEYCODE(keycode)) || IS_CONSUMER_KEYCODE(QK_MODS_GET_BASIC_KEYCODE(keycode))) {
      if (record->event.pressed) {
        add_mods(QK_MODS_GET_MODS(keycode));
        send_keyboard_report();
        wait_ms(2);
        register_code(QK_MODS_GET_BASIC_KEYCODE(keycode));
        return false;
      } else {
        wait_ms(2);
        del_mods(QK_MODS_GET_MODS(keycode));
      }
    }
    break;
    case ST_MACRO_0:
    if (record->event.pressed) {
      SEND_STRING(SS_TAP(X_P)SS_DELAY(25)  SS_TAP(X_R)SS_DELAY(25)  SS_TAP(X_I)SS_DELAY(25)  SS_TAP(X_N)SS_DELAY(25)  SS_TAP(X_T)SS_DELAY(25)  SS_LSFT(SS_TAP(X_9))SS_DELAY(25)  SS_TAP(X_F)SS_DELAY(25)  SS_LSFT(SS_TAP(X_QUOTE))SS_DELAY(25)  SS_LSFT(SS_TAP(X_LBRC))SS_DELAY(25)  SS_LSFT(SS_TAP(X_RBRC))SS_DELAY(25)  SS_LSFT(SS_TAP(X_QUOTE))SS_DELAY(25)  SS_LSFT(SS_TAP(X_0))SS_DELAY(25)  SS_TAP(X_LEFT)SS_DELAY(25)  SS_TAP(X_LEFT)SS_DELAY(25)  SS_TAP(X_LEFT));
    }
    break;
    case ST_MACRO_1:
    if (record->event.pressed) {
      SEND_STRING(SS_TAP(X_BSLS)SS_DELAY(100)  SS_TAP(X_BSLS)SS_DELAY(100)  SS_TAP(X_F));
    }
    break;
    case ST_MACRO_2:
    if (record->event.pressed) {
      SEND_STRING(SS_LSFT(SS_RALT(SS_TAP(X_6)))SS_DELAY(50)  SS_TAP(X_S));
    }
    break;
    case ST_MACRO_3:
    if (record->event.pressed) {
      SEND_STRING(SS_LCTL(SS_LSFT(SS_TAP(X_U)))SS_DELAY(20)  SS_TAP(X_3)SS_DELAY(20)  SS_TAP(X_F)SS_DELAY(20)  SS_TAP(X_1)  SS_DELAY(20) SS_TAP(X_ENTER));
    }
    break;
    case ST_MACRO_4:
    if (record->event.pressed) {
      SEND_STRING(SS_LCTL(SS_LSFT(SS_TAP(X_U)))SS_DELAY(20)  SS_TAP(X_3)SS_DELAY(20)  SS_TAP(X_D)SS_DELAY(20)  SS_TAP(X_1)  SS_DELAY(20) SS_TAP(X_ENTER));
    }
    break;
    case ST_MACRO_5:
    if (record->event.pressed) {
      SEND_STRING(SS_RCTL(SS_RSFT(SS_TAP(X_COMMA)))SS_DELAY(5)  SS_RCTL(SS_RSFT(SS_TAP(X_COMMA)))SS_DELAY(5)  SS_RCTL(SS_RSFT(SS_TAP(X_COMMA)))SS_DELAY(5)  SS_RCTL(SS_RSFT(SS_TAP(X_COMMA)))SS_DELAY(5)  SS_RCTL(SS_RSFT(SS_TAP(X_COMMA)))SS_DELAY(5)  SS_RCTL(SS_RSFT(SS_TAP(X_COMMA)))SS_DELAY(5)  SS_RCTL(SS_RSFT(SS_TAP(X_COMMA)))SS_DELAY(5)  SS_RCTL(SS_RSFT(SS_TAP(X_COMMA)))SS_DELAY(5)  SS_RCTL(SS_RSFT(SS_TAP(X_COMMA)))SS_DELAY(5)  SS_RCTL(SS_RSFT(SS_TAP(X_COMMA)))SS_DELAY(5)  SS_RCTL(SS_RSFT(SS_TAP(X_COMMA)))SS_DELAY(5)  SS_RCTL(SS_RSFT(SS_TAP(X_COMMA)))SS_DELAY(5)  SS_TAP(X_COMMA)SS_DELAY(5)  SS_TAP(X_COMMA)SS_DELAY(5)  SS_TAP(X_COMMA)SS_DELAY(5)  SS_TAP(X_COMMA)SS_DELAY(5)  SS_TAP(X_COMMA)SS_DELAY(5)  SS_TAP(X_COMMA)SS_DELAY(5)  SS_TAP(X_COMMA)SS_DELAY(5)  SS_TAP(X_COMMA)SS_DELAY(5)  SS_TAP(X_COMMA)SS_DELAY(5)  SS_TAP(X_COMMA)SS_DELAY(5)  SS_TAP(X_COMMA)SS_DELAY(5)  SS_TAP(X_COMMA));
    }
    break;
    case ST_MACRO_6:
    if (record->event.pressed) {
      SEND_STRING(SS_LCTL(SS_LSFT(SS_TAP(X_F)))SS_DELAY(100)  SS_TAP(X_1));
    }
    break;
    case ST_MACRO_7:
    if (record->event.pressed) {
      SEND_STRING(SS_LCTL(SS_LSFT(SS_TAP(X_U)))SS_DELAY(10)  SS_TAP(X_1)SS_DELAY(10)  SS_TAP(X_3)SS_DELAY(10)  SS_TAP(X_0)SS_DELAY(10)  SS_TAP(X_B)SS_DELAY(10)  SS_TAP(X_A)  SS_DELAY(10) SS_TAP(X_ENTER));
    }
    break;

    case AUTOCLK_R:                 /* right button */
    if (record->event.pressed) {
      autoclick_start(&ac_right);
    } else {
      autoclick_stop(&ac_right);
    }
    return false;

    case AUTOCLK_L:                 /* left button */
    if (record->event.pressed) {
      autoclick_start(&ac_left);
    } else {
      autoclick_stop(&ac_left);
    }
    return false;

    case DUALHOLD:                  /* momentary: down while held */
    if (record->event.pressed) {
      dualhold_start();
    } else {
      dualhold_stop();
    }
    return false;

    case DUALTOG:                   /* latch: tap on, tap off */
    if (record->event.pressed) {    /* act on press, ignore release */
      if (dh_active) {
        dualhold_stop();
      } else {
        dualhold_start();
      }
    }
    return false;

    case RGB_SLD:
        if (rawhid_state.rgb_control) {
            return false;
        }
        if (record->event.pressed) {
            rgblight_mode(1);
        }
        return false;
  }
  return true;
}


void matrix_scan_user(void) {
  autoclick_task(&ac_right);
  autoclick_task(&ac_left);
}
