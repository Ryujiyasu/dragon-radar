#include "theme.h"

/* 7 ドラゴンボールはアニメ・玩具では全て同じ金色オレンジ。
 * ただ完全同色だと描画上の差別化が出ないので「番号(星の数)」をドット内に重ねるのと、
 * 微妙な明度差で識別性を補助する。 */
const lv_color_t dr_tag_palette[DR_MAX_TAGS] = {
    {.red = 0xFF, .green = 0xC8, .blue = 0x00}, /* 1 ★ ドラゴンボール金 */
    {.red = 0xFF, .green = 0xBE, .blue = 0x00}, /* 2 ★★ */
    {.red = 0xFF, .green = 0xB4, .blue = 0x00}, /* 3 ★★★ */
    {.red = 0xFF, .green = 0xAA, .blue = 0x00}, /* 4 ★★★★ */
    {.red = 0xFF, .green = 0xA0, .blue = 0x00}, /* 5 ★★★★★ */
    {.red = 0xFF, .green = 0x96, .blue = 0x00}, /* 6 ★★★★★★ */
    {.red = 0xFF, .green = 0x8C, .blue = 0x00}, /* 7 ★★★★★★★ */
};
