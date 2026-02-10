/**
 * LVGL Configuration for Monopoly Electronic V2
 * Target: ESP32-S3 + ST7789 320x240 + XPT2046 touch
 * LVGL v9.x
 *
 * Only settings that differ from defaults are listed here.
 * Everything else uses LVGL's built-in defaults via lv_conf_internal.h.
 */

#if 1 /* Set to 1 to enable content */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/* Color depth: 16 = RGB565 (matches our TFT) */
#define LV_COLOR_DEPTH 16

/*====================
   MEMORY SETTINGS
 *====================*/

/* Use LVGL's built-in memory manager */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN

/* 48 KB heap for LVGL objects */
#define LV_MEM_SIZE (48U * 1024U)

/*====================
   DISPLAY SETTINGS
 *====================*/

/* Refresh period in ms (~30 fps) */
#define LV_DEF_REFR_PERIOD 33

/* Default DPI (320x240 on ~2.4 inch) */
#define LV_DPI_DEF 130

/*====================
   OPERATING SYSTEM
 *====================*/

#define LV_USE_OS   LV_OS_NONE

/*====================
   DRAW SETTINGS
 *====================*/

/* We don't use TFT_eSPI's built-in LVGL driver – we implement our own flush */
#define LV_USE_TFT_ESPI 0

/*====================
   FONT SETTINGS
 *====================*/

/* Enable Montserrat fonts at sizes we need */
#define LV_FONT_MONTSERRAT_12  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  1
#define LV_FONT_MONTSERRAT_20  1
#define LV_FONT_MONTSERRAT_24  1
#define LV_FONT_MONTSERRAT_28  1

/* Default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*====================
   THEME
 *====================*/

#define LV_USE_THEME_DEFAULT    1
#define LV_THEME_DEFAULT_DARK   1

/*====================
   LOGGING
 *====================*/

#define LV_USE_LOG      1
#define LV_LOG_LEVEL    LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF   1

/*====================
   WIDGET OVERRIDES
 *====================*/

/* Explicitly enable what we need (most are ON by default in v9) */
#define LV_USE_LABEL        1
#define LV_USE_BTN          1
#define LV_USE_BTNMATRIX    1
#define LV_USE_BAR          1
#define LV_USE_SLIDER       1
#define LV_USE_TEXTAREA     1
#define LV_USE_KEYBOARD     1
#define LV_USE_ROLLER       1
#define LV_USE_CHECKBOX     1
#define LV_USE_SWITCH       1
#define LV_USE_LIST         1
#define LV_USE_SPINNER      1
#define LV_USE_MSGBOX       1
#define LV_USE_SPINBOX      1

/* Disable widgets we don't need to save flash */
#define LV_USE_CHART        0
#define LV_USE_CALENDAR     0
#define LV_USE_METER        0
#define LV_USE_SCALE        0

#endif /* LV_CONF_H */

#endif /* Enable content */
