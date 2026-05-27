# ESP32-P4 × LVGL で「バンダイ ドラゴンレーダー」風 UI を本気で再現する

## 要約

ESP32-P4 + 800×800 円形 LCD (Waveshare 3.4C / IPS) という贅沢なハードで、**1980 年代バンダイから出ていた玩具版「ドラゴンレーダー」の画面表示**を再現してみた記録。

ポイントは 4 つ:

1. **色** ── 緑 CRT 背景 + 太い黒グリッド + ゴールド数字 + コーラル三角ポインタ
2. **三角ポインタ** ── 「現在地」を表す塗り潰し三角を LVGL の `lv_obj` 矩形スタックで描く (LVGL 9 にはネイティブの三角プリミティブが無いため)
3. **スイープ** ── 円弧 30 度幅の `lv_arc` を回し続ける
4. **光点 (ドラゴンボール)** ── 距離→対数スケール、AoA 方位→極座標変換で配置

LVGL は工業 HMI に最適化されていて「アニメ・玩具風の温かみある画面」を作るのが意外と難しい。本記事はその難易度をどう越えたかの記録。

## ハードと前提

- **MCU**: ESP32-P4 (RISC-V dual-core 400MHz、PSRAM 32MB)
- **LCD**: 800×800 round IPS (Waveshare 3.4C)
- **フレームワーク**: ESP-IDF v5.5.4、LVGL 9.5.0、esp_lvgl_adapter 経由
- **描画モード**: triple partial buffer + tear-avoid

LCD は MIPI-DSI で繋がっていて、フレームバッファは PSRAM に乗る。Waveshare BSP (`waveshare/esp32_p4_wifi6_touch_lcd_xc`) を managed component で取り込めば LVGL までの配線は数行で済む。

## カラーパレット

玩具版の写真を観察してパレットを取った:

```c
// theme.h
#define DR_COLOR_BG       lv_color_hex(0x2A9040)  // 緑 CRT
#define DR_COLOR_GRID     lv_color_hex(0x001008)  // ほぼ黒
#define DR_COLOR_SWEEP    lv_color_hex(0xA0FFB0)  // スイープのトレイル
#define DR_COLOR_TEXT     lv_color_hex(0xFFD700)  // ゴールド (数字 / ラベル)
#define DR_COLOR_DOT      lv_color_hex(0xFFC800)  // ドラゴンボール (オレンジゴールド)
#define DR_COLOR_POINTER  lv_color_hex(0xFF6E50)  // コーラル (中央三角)
```

ポイントは **背景の緑をくすませる** こと。サチった #00FF00 を使うと「LED マトリクス」っぽくなり玩具感が消える。 #2A9040 くらいの**やや暗くて青みのある緑**が CRT 蛍光体っぽくて良い。

## グリッド

LVGL の標準では「画面全体に等間隔の格子線」を引くプリミティブは無いので `lv_line_create` を縦横ループで配置する。

```c
#define GRID_STEP 50
for (int x = GRID_STEP; x < DR_SCREEN_SIZE; x += GRID_STEP) {
    static lv_point_precise_t pts[20][2];
    int i = x / GRID_STEP - 1;
    pts[i][0].x = x; pts[i][0].y = 0;
    pts[i][1].x = x; pts[i][1].y = DR_SCREEN_SIZE;
    lv_obj_t *line = lv_line_create(parent);
    lv_line_set_points(line, pts[i], 2);
    lv_obj_set_style_line_color(line, DR_COLOR_GRID, 0);
    lv_obj_set_style_line_width(line, 2, 0);
    lv_obj_set_style_line_opa(line, LV_OPA_80, 0);
}
```

`lv_point_precise_t` は **static** にしないと描画タイミングでスタック上のメモリが消えて画面に変な線が現れる。LVGL のプリミティブはポインタを保持するだけで、内部コピーしないことが多いので注意。

## 三角ポインタ ── 一番苦戦したパーツ

LVGL 9 に **塗り潰し三角プリミティブは無い**。`lv_canvas` で自前描画してもいいが、800×800 のフルキャンバスを PSRAM に保持するのは重い。

そこで採用したのが **水平スキャンラインで矩形を積む方式**:

```c
#define PT_SIZE 16     // 三角の半幅
#define PT_STEP 2      // スキャンラインの厚さ
const int y_top    = DR_CENTER - PT_SIZE;
const int y_bottom = DR_CENTER + PT_SIZE / 2;
const int height   = y_bottom - y_top;

for (int y = y_top; y <= y_bottom; y += PT_STEP) {
    int dy = y - y_top;
    int half_w = (dy * PT_SIZE) / height;
    if (half_w < 1) half_w = 1;

    lv_obj_t *seg = lv_obj_create(parent);
    lv_obj_remove_style_all(seg);
    lv_obj_set_size(seg, half_w * 2, PT_STEP);
    lv_obj_set_pos(seg, DR_CENTER - half_w, y);
    lv_obj_set_style_bg_color(seg, DR_COLOR_POINTER, 0);
    lv_obj_set_style_bg_opa(seg, LV_OPA_COVER, 0);
}
```

頂点が上、底辺が下の塗り潰し三角を、PT_STEP=2 ピクセル幅の矩形を ~12 個積んで再現する。**メモリは矩形 12 個分しか食わない** (内部的に lv_obj は数百バイト)。LVGL の合成エンジンが矩形描画はべらぼうに速いので描画コストもタダ同然。

回転させたい場合は `lv_obj_set_style_transform_angle` でグループ全体を回せばよい。三角ポインタを「機体の向きに合わせて回転」させる用途にも使える。

## スイープアーク

円弧を回すのは LVGL のお家芸:

```c
lv_obj_t *arc = lv_arc_create(parent);
lv_obj_set_size(arc, DR_RADAR_RING_R2 * 2, DR_RADAR_RING_R2 * 2);
lv_obj_align(arc, LV_ALIGN_CENTER, 0, 0);
lv_arc_set_bg_angles(arc, 0, 360);
lv_arc_set_rotation(arc, 270);
lv_arc_set_angles(arc, 0, 30);  // 30 度幅
lv_obj_set_style_arc_color(arc, DR_COLOR_SWEEP, LV_PART_INDICATOR);
lv_obj_set_style_arc_width(arc, DR_RADAR_RING_R2, LV_PART_INDICATOR);
lv_obj_set_style_arc_opa(arc, LV_OPA_30, LV_PART_INDICATOR);

lv_anim_t a;
lv_anim_init(&a);
lv_anim_set_var(&a, arc);
lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_arc_set_rotation);
lv_anim_set_values(&a, 270, 270 + 360);
lv_anim_set_duration(&a, 1500);
lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
lv_anim_start(&a);
```

`arc_width = R2` (リング全径) にして、塗り部分の不透明度を 30% に落とすと「スポットライトが回転する」風になる。トレイル感を出したいなら 2 本目のアークを反対側に置いて opa=10% にする手もある。

## 光点 (ドラゴンボール) 配置

UWB ranging から流れてくる `distance_mm` と `azimuth_deg` を画面座標に変換する:

```c
static int32_t distance_to_radius_px(uint16_t distance_mm) {
    if (distance_mm == 0) return 0;
    if (distance_mm >= DR_RANGE_MAX_MM) return DR_RADAR_RING_R2;
    float ratio = logf(1.0f + (float)distance_mm / 1000.0f)
                / logf(1.0f + (float)DR_RANGE_MAX_MM / 1000.0f);
    return (int32_t)(DR_RADAR_RING_R2 * ratio);
}

static void polar_to_cartesian(uint16_t distance_mm, int16_t azimuth_deg,
                               int32_t *x, int32_t *y) {
    int32_t r = distance_to_radius_px(distance_mm);
    float az_rad = azimuth_deg * M_PI / 180.0f;
    *x = DR_CENTER + (int32_t)(r * sinf(az_rad));
    *y = DR_CENTER - (int32_t)(r * cosf(az_rad));
}
```

距離 → 半径は **対数スケール**にする。線形だと「近い時は中心にくっつく、遠い時はリングの端に張り付く」となるが、人間の空間感覚に合うのは対数。1m での解像度を大きく取り、10m の精度を犠牲にする。

光点本体は丸オブジェクトに **シャドウで発光感**を出す:

```c
lv_obj_t *dot = lv_obj_create(parent);
lv_obj_set_size(dot, 30, 30);
lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
lv_obj_set_style_bg_color(dot, DR_COLOR_DOT, 0);
lv_obj_set_style_shadow_color(dot, DR_COLOR_DOT, 0);
lv_obj_set_style_shadow_width(dot, 28, 0);
lv_obj_set_style_shadow_opa(dot, LV_OPA_70, 0);
```

`shadow_width = 28` (本体径とほぼ同じ) で**等方ハロー**ができ、ドラゴンボールの「内側から光ってる」雰囲気が出る。

## ハマりどころメモ

| 症状 | 原因 / 対処 |
|---|---|
| 起動時にリンカが `--enable-non-contiguous-regions discards section ... 21KB` | ESP32-P4 v1.x シリコンで `CONFIG_SPIRAM_XIP_FROM_PSRAM` 有効だと IRAM 溢れ → 無効化 |
| LCD が真っ黒 | `CONFIG_LV_USE_PERF_MONITOR=n`、`COMPILER_OPTIMIZATION_SIZE=y`、L2 cache 128KB に落とす |
| 描画がカクつく | `LV_COLOR_DEPTH=16`、frame buffer を PSRAM に置く、`triple_partial` モード |
| 緑がサチった LED 色になる | パレットを `#2A9040` 系のくすんだ緑に。`#00FF00` は使わない |

## 動かしてみた

LCD のフォトを撮って横で玩具のドラゴンレーダーと比較した時、**「電池入れて 30 年経った玩具」っぽい質感**が出てしまった (緑が少しくすんでいる + 黒グリッドが太い + フォントがゴールド) ── 結果としてその"古色感"が逆に良かった。

LVGL は HMI 用ライブラリだけど、**プリミティブを工夫すれば「玩具・アニメ的な温かみのある UI」も十分作れる**。三角プリミティブが無いから諦める前に、矩形スタックで作れないか考えてみる価値はある。
