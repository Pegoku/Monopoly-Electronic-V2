/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "lvgl.h"
#include "app_hal.h"

/* Lightweight RNG to avoid std headers on embedded toolchains */
static unsigned long __rng_state = 1UL;
static inline void rng_seed(unsigned long s) { __rng_state = s ? s : 1UL; }
static inline unsigned long rng_next(void) {
  /* 32-bit LCG: Numerical Recipes constants */
  __rng_state = __rng_state * 1664525UL + 1013904223UL;
  return __rng_state;
}

/* Shared helpers (visible to both Arduino and non-Arduino) */
static void btn_event_cb(lv_event_t * e);
static void create_random_buttons();


#ifdef ARDUINO
#include <Arduino.h>

void setup() {
  lv_init();
  hal_setup();

  /* Seed RNG using LVGL tick to keep it portable */
  rng_seed((unsigned long)lv_tick_get());

  create_random_buttons();
}

void loop() {
  hal_loop();  /* Do not use while loop in this function */
}

#else

/* Non-Arduino branch */

/**
 * Create a button with a label and react on click event.
 */
/* create_random_buttons is defined below as a shared helper */

int main(void)
{
	lv_init();

	hal_setup();

  /* Seed RNG using LVGL tick to keep it portable */
  rng_seed((unsigned long)lv_tick_get());

  create_random_buttons();

	hal_loop();
}

#endif /*ARDUINO*/

/* Shared button event callback (Arduino and non-Arduino) */
static void btn_event_cb(lv_event_t * e)
{
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

  lv_obj_t * label = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
  if (!label) return;

  unsigned value = (unsigned)(rng_next() % 10001UL); /* 0..10000 inclusive */
  lv_label_set_text_fmt(label, "%u", value);
}

/* Shared UI builder to create 5 random-value buttons */
static void create_random_buttons()
{
  /* Create 5 buttons laid out in 2 rows (3 on first row, 2 on second) */
  const int btn_w = 120;
  const int btn_h = 60;
  const int margin = 10;

  lv_obj_t * scr = lv_screen_active();

  for (int i = 0; i < 5; ++i) {
    lv_obj_t * btn = lv_button_create(scr);

    int row = (i < 3) ? 0 : 1;
    int col = (i < 3) ? i : (i - 3);
    int x = margin + col * (btn_w + margin);
    int y = margin + row * (btn_h + margin);

    lv_obj_set_size(btn, btn_w, btn_h);
    lv_obj_set_pos(btn, x, y);

    lv_obj_t * label = lv_label_create(btn);
    unsigned value = (unsigned)(rng_next() % 10001UL); /* 0..10000 inclusive */
    lv_label_set_text_fmt(label, "%u", value);
    lv_obj_center(label);

    /* Only handle click events; pass label as user data for quick updates */
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, label);
  }
}