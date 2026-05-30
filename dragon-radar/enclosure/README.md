# Dragon Radar UWB — 筐体 (Enclosure)

DigiKey Make ONE Challenge 2026 / プレイヤー機の 3D プリント筐体。

- CAD: **OpenSCAD** (パラメトリック、git 管理)
- 印刷: **FDM / PLA / ベッド ~220mm** (ø 一体造形、分割なし)
- ソース: [`dragon-radar-enclosure.scad`](dragon-radar-enclosure.scad)

## 現状: v0.1 スケルトン

3 部品クラムシェル + ボード mock。嵌合関係とサイズ感を固めた段階。
細部 (ネジボス・コネクタ角度・ボタンボス) は **ボード現物が届いてから実測 → 確定** する。

```
[前面ベゼル] 表示窓 ø91.6 でガラス縁を抑える
[本体タブ]   側壁 + 着座シェルフ + マウントボス + コネクタ/ボタン開口
[バックカバー] 配線通気 + ネジ留め
```

## レンダリング / 書き出し

OpenSCAD GUI で開き、先頭の `part` 変数を切替えてプレビュー。
CLI での STL / PNG 書き出し例 (このリポジトリで使った AppImage 展開バイナリ):

```bash
# 各部品を印刷用 STL に
for p in bezel body back; do
  openscad -o dr_$p.stl -D "part=\"$p\"" dragon-radar-enclosure.scad
done

# 嵌合確認の PNG
openscad --camera=0,0,-12,62,0,25,300 --imgsize=900,700 \
  -o assembly.png -D 'part="assembly"' dragon-radar-enclosure.scad
```

`part` の値: `assembly` / `bezel` / `body` / `back` / `board`

## 寸法の出典と確度

### [確定] Waveshare 3.4C 実寸 (wiki 寸法図 + CNX 記事)

| 項目 | 値 | 出典 |
|---|---|---|
| ガラス/前面外形 | **ø115 mm** | CNX「115×115 Outline」 |
| 有効表示エリア | **ø87.6 mm** (800×800, 3.4") | CNX「87.6×87.6 Display area」 |
| 前面ガラス厚 | ~6 mm | wiki 側面図 (4C 図から類推) |
| モジュール総厚 | ~15 mm | wiki 側面図 |

> 注: wiki 寸法図 (ø126 / ø101.52 / 厚15) は **4 インチ版 (4C)** のもの。
> 3.4C はそれより一回り小さい ø115 / ø87.6。仕様書の「ø110」想定より実機は大きく、
> **筐体外径は ~ø121 mm**(下記計算)になる。`dragon-radar-spec.md` の筐体寸法は要更新。

出典:
- https://www.waveshare.com/wiki/ESP32-P4-WIFI6-Touch-LCD-3.4C
- https://www.cnx-software.com/2025/06/01/esp32-p4-development-board-features-3-4-inch-or-4-inch-round-ips-touchscreen-display/

### [要実測 MEASURE] ボード到着後に現物確定する項目

`.scad` 内で `[MEASURE]` 印を付けた変数。実測値を入れれば全体が追従する。

| 変数 | 仮置き | 確定方法 |
|---|---|---|
| `mount_count` / `mount_pcd` / `mount_hole_dia` / `mount_angle0` | 4 / ø105 / ø2.5 / 45° | マウント穴を実測 (PCD・本数・径・基準角) |
| USB-C / USB-OTG / microSD のリム角度・開口寸法 | `connector_cutout()` 3 箇所が仮 | 各コネクタの周方向位置と高さを実測 |
| `back_clear` | 8 mm | 背面コネクタ群の最大突出 + 電池/配線スペース |
| `button_z` | スタック中央 | MKBKLLJY ボタンの実装高さ |

### 筐体派生寸法 (現状の仮値での計算結果)

| 寸法 | 値 |
|---|---|
| 内壁径 `inner_dia` | ~116.2 mm |
| **外径 `outer_dia`** | **~121.0 mm** |
| 表示窓径 `window_dia` | ~91.6 mm (有効表示 ø87.6 を完全露出) |
| **筐体総厚 `outer_depth`** | **~27.9 mm** (仕様目標 30mm 以内 ✅) |

## 印刷設定 (FDM / PLA、初版の目安)

| 項目 | 値 | 備考 |
|---|---|---|
| ノズル / 層厚 | 0.4mm / 0.2mm | 側壁 `wall=2.4` は 0.4×6 で割り切れる |
| 壁ライン数 | 3–4 | |
| インフィル | 15–20% gyroid | |
| サポート | ベゼル窓まわり程度 | 各部品は概ね自立印刷向き |
| 向き | ベゼル=窓を上 / 本体=開口を上 / バック=平面下 | |

## 次のステップ (ボード到着前にできる事 / 到着後)

- [ ] ボード到着 → `[MEASURE]` 変数を実測値で確定
- [ ] マウントボスをネジ種 (M2.5 セルフタップ or インサート) に合わせて確定
- [ ] ボタンボス (MKBKLLJY ø12、緑LED+MOSFET 配線) を上部リムに作り込み
- [ ] ベゼル⇔本体の固定方式決定 (スナップフィット or ネジ)
- [ ] ストラップ/手持ち形状、滑り止め
- [ ] `dragon-radar-spec.md` の筐体寸法を ø110→ø121 に更新
