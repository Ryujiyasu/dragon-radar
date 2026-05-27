# Dragon Radar UWB

> ドラゴンボールの「ドラゴンレーダー」を現代の UWB (Ultra-Wide Band) 測距技術で実物として作るプロジェクト。
> DigiKey Make ONE Challenge 2026 (応募締切 **2026-06-22**) 出展作品。

円形 LCD を載せた手持ち端末が、周囲に散らばった UWB タグ (= ドラゴンボール) の **距離 + 方向 (AoA)** を 5 cm 精度で表示する。UI は 1980 年代バンダイの玩具版ドラゴンレーダーをオマージュ。

詳細仕様は [`dragon-radar-spec.md`](dragon-radar-spec.md)。背景・設計判断・進捗の物語は本 README とブログ記事 ([Related reading](#related-reading)) で記録する。

---

## What works today (2026-05-19 時点)

| 項目 | 状態 | 観測値 |
|---|---|---|
| 2BP ↔ 2DK ranging 成立 | ✅ | 50–60 秒で 200+ サンプル、両端 avg 距離差 < 0.5 cm |
| AoA 方位取得 | ✅ | -44° 安定 (左方向に 2DK 配置時) |
| ESP32-P4 で Bandai 風 UI 描画 | ✅ | 800×800、LVGL 9.5、5 Hz 更新 |
| QN9090 カスタム FW で `RADAR,t=...` ASCII 出力 | ✅ | 3 Mbps UART、~5 Hz |
| ESP32-P4 ↔ 2BP 物理 UART 配線 | ⏳ | TP48→GPIO47 / TP47→GPIO48 / GND、配線待ち |
| 2DK タグ × 7 個拡張 | 未 | Phase 3 以降 |
| 効果音 + BGM | 未 | Phase 4 以降 |

---

## 採用アーキテクチャ: Path C (二段構え)

```
Stage 1 (~6/22)  : Path C (QN9090 カスタム FW)         → DigiKey 応募
Stage 2 (~9月)   : 2DK タグ × 7 個拡張 + 演出           → Maker Faire Tokyo 2026
Stage 3 (将来)   : Path B (ESP32 ↔ SR150 SPI 直結) 移植 → QN9090 を取り払いコスト/サイズ削減
```

Path C は QN9090 (Murata EVK 標準搭載の host MCU) をそのまま使い、その上で動くファームを NXP UWBIOT SDK ベースで自作する路線。

---

## ハードウェア

| 役割 | 型番 | 数 |
|---|---|---|
| プレイヤー機 UI / ホスト | **Waveshare ESP32-P4-WIFI6-Touch-LCD-3.4C** (800×800 round IPS) | 1 |
| 旧 UI 予備 | Waveshare ESP32-S3-Touch-LCD-1.85 | 1 |
| プレイヤー機 UWB (initiator + AoA) | **Murata Type 2BP EVK Rev 4.1** (SR150 + QN9090) | 1 |
| タグ (responder) | **Murata Type 2DK EVK** (SR040 + QN9090) | 2 |
| デバッガ (任意) | NXP MCU-Link Pro | 1 |
| 書込ツール | **`spsdk` (`dk6prog`)** — USB ISP 経由、SWD 不要 | — |

> ⚠️ 技適制約: ESP32-P4 ボード上の ESP32-C6 (WiFi6 コンパニオン) は技適マーク未確認のため、**WiFi/BT は使用しない方針**。2BP ↔ ESP32-P4 は物理配線で接続する。詳細は [#5 技適記事](https://yasu-home.com/?p=37) 参照。

---

## システム構成 (Path C / Stage 1)

```
┌──────────────────────────────────┐         ┌───────────────────────────────┐
│   ESP32-P4 (Waveshare 3.4C)      │         │   Murata 2BP EVK Rev 4.1      │
│                                  │  TP48   │                               │
│   LVGL 9 (Bandai 風 UI)          │ ◄────── │   QN9090 (UCI host)           │
│   uwb_uart_task (3 Mbps)         │ GPIO47  │     │                         │
│   parse RADAR ASCII              │   GND   │   SR150  ◄── UWB 6.5GHz ──┐   │
│   polar → radar pixel            │ ──────  │     (initiator + 3-ant AoA)│   │
└──────────────────────────────────┘         └───────────────────────────────┘
                                                                              │
                                              ┌───────────────────────────────┘
                                              ▼
                                    ┌───────────────────────────────┐
                                    │   Murata 2DK EVK × 2          │
                                    │   QN9090 (Murata standalone)  │
                                    │   SR040 (responder)           │
                                    └───────────────────────────────┘
```

UART payload format (QN9090 が 3 Mbps で吐く 1 行):

```
APP     :INFO :RADAR,t=0,d=21,az=-44.18,el=-60.0,st=0
                       │   │      │           │       │
                       │   │      │           │       └── status: 0=OK
                       │   │      │           └────────── elevation (Q9.7 deg)
                       │   │      └────────────────────── azimuth   (Q9.7 deg)
                       │   └────────────────────────────── distance (cm)
                       └──────────────────────────────── tag id
```

---

## SDK バージョン整合 (最重要)

2BP (SR150) と 2DK (SR040) は **UCI プロトコルの世代を揃えないと ranging が成立しない**。詳細は [#3 UCI 世代差記事](https://yasu-home.com/?p=35)。

| EVK | 採用 FW | UCI 世代 | session ID |
|---|---|---|---|
| 2BP (SR150) | NXP UWBIOT SDK **v04.04.03** ベースのカスタム FW | **1.31** | 0x11223344 |
| 2DK (SR040) | Murata Standalone **v04.03.14** プリビルド (校正済) | **1.31** | 0x11223344 |

UCI v2.0 系 (v04.06.00+) と v1.x 系を混在させると、RANGE_DATA_NTF の `session ID` フィールドの意味が変わる (handle vs ID) ためペアリング不成立。

---

## ディレクトリ

```
dragon-radar/
├── firmware/
│   ├── esp32-p4/                   # ESP-IDF v5.5.4 (UI + UART parser + game)
│   │   ├── main/
│   │   │   ├── ui/                 # radar_view, theme (Bandai colors)
│   │   │   ├── uwb/                # uwb_uart (3Mbps RADAR parser), uwb_filter
│   │   │   ├── game/               # ドラゴンボール検知ゲーム
│   │   │   ├── audio/              # I2S WAV (BGM / SE)
│   │   │   └── storage/            # microSD (FATFS)
│   │   └── sdkconfig.defaults
│   ├── 2bp-fw/                     # 2BP の QN9090 用カスタム FW (Path C)
│   ├── 2dk-fw/                     # 2DK の QN9090 用 FW (将来 Responder 自前ビルド)
│   └── 2bp-config/                 # ★ NDA 配下、gitignore
│       ├── docs/                   # Murata 提供 PDF
│       ├── sdk/                    # NXP UWBIOT SDK + Murata パッチ
│       ├── firmware-backups/       # 工場 FW dk6prog ダンプ
│       └── firmware-builds/        # 自前ビルド済みバイナリ
├── docs/
│   └── 2bp-protocol.md             # ESP32 ↔ QN9090 ASCII プロトコル仕様
├── assets/
│   ├── audio/                      # WAV (神龍 / ウーロン / SE)
│   └── images/                     # PNG (光点, 龍, 星)
└── tools/                          # PC 側スクリプト
```

---

## ビルド

### ESP32-P4 ファーム

```bash
. ~/esp/esp-idf-v5.5/export.sh
cd dragon-radar/firmware/esp32-p4
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

GPIO 割当 ([uwb_uart.h](dragon-radar/firmware/esp32-p4/main/uwb/uwb_uart.h)):

| ESP32-P4 GPIO | 2BP EVK TP | 信号 |
|---|---|---|
| GPIO 47 (UART1 RX) | **TP48** | RADAR ASCII (3 Mbps) |
| GPIO 48 (UART1 TX) | **TP47** | 制御送信 (optional) |
| GND | GND (TP29 等) | — |

⚠️ 番号交差注意: 2BP **TP48** ↔ ESP32 **GPIO47**、2BP **TP47** ↔ ESP32 **GPIO48**。

### 2BP QN9090 カスタム FW (NXP UWBIOT SDK)

MCUXpresso IDE の headless build を使う ([#2 MCUXpresso 記事](https://yasu-home.com/?p=34))。

```bash
xvfb-run --auto-servernum --server-args="-screen 0 1024x768x24" \
  /usr/local/mcuxpressoide-25.6.136/ide/mcuxpressoide \
  -nosplash \
  -application org.eclipse.cdt.managedbuilder.core.headlessbuild \
  -data /tmp/mcux-ws \
  -import dragon-radar/firmware/2bp-config/sdk/UWBIOT_SR150_v04.04.03_MCUx/uwbiot-top/project/RhodesV4_SE \
  -cleanBuild "RhodesV4_SE/Debug"
```

出力: `.../project/RhodesV4_SE/Debug/RhodesV4_SE.bin` (約 350 KB、9.6 秒)。

### 2BP に書込 (USB ISP / SWD 不要)

`dk6prog` (SPSDK 同梱) で書き込む ([#1 dk6prog 記事](https://yasu-home.com/?p=33))。

```bash
# 1. EVK のみ単独で USB 接続、ISP ボタン押しながらリセット
dk6prog listdev                                                    # DM86TTWC を確認
dk6prog -d DM86TTWC read 0 0x9DE00 -o factory_backup.bin           # 工場 FW バックアップ (1 度だけ)
dk6prog -d DM86TTWC erase 0 0x9DE00
dk6prog -d DM86TTWC write 0 dragon-radar/firmware/2bp-config/firmware-builds/2bp_dragon_radar_v0.1.bin
```

### 2DK は Murata 出荷 FW のまま使う

2DK は Murata が標準で `2dk_controlee_v04.03.14.bin` を焼いて出荷しており、これがそのまま v1.31 UCI の responder として動く。**書換不要**。

---

## 開発フェーズ

- [x] **Phase 0**: 環境準備 (NXP SDK / MCUXpresso / SPSDK / Waveshare BSP) + 工場 FW バックアップ
- [x] **Phase 1**: ESP32-P4 + LVGL Bandai レーダー UI (ダミーデータ) ✅ 2026-05-19
- [x] **Phase 2a**: NXP UWBIOT SDK v04.04.03 ヘッドレスビルド検証 ✅ 5/19
- [x] **Phase 2b**: `printRangingData` 改造で `RADAR,t=...` ASCII 出力 ✅ 5/19
- [x] **Phase 2c**: 2BP にカスタム FW 書込 + 3 Mbps UART で実機 RADAR 出力確認 ✅ 5/19
- [x] **Phase 2d-1〜4**: EVK ピン特定 + ESP32-P4 GPIO 選定 + UART parser + LVGL 連携実装 ✅ 5/19
- [ ] **Phase 2d-5**: 物理配線 (TP48→GPIO47 等) + 実機テスト (LCD に実距離+AoA 表示)
- [ ] **Phase 3**: 2DK タグ拡張 (2 個 → 7 個) + round-robin スケジューラ
- [ ] **Phase 4**: 音声 (神龍/ウーロン) + SE + BGM + 検知ゲーム
- [ ] **Phase 5**: 筐体 CAD + 3D プリント + 組立
- [ ] **Phase 6**: デモ動画撮影 + DigiKey 応募 (~6/22)
- [ ] **Phase 7**: Maker Faire Tokyo 2026 (~9/5-6) 持込

---

## 制約と教訓

### 技適制約

Waveshare ESP32-P4-WIFI6-Touch-LCD-3.4C 上の ESP32-C6 (WiFi6 コンパニオン) は技適マーク未確認。
公開展示 (DigiKey、Maker Faire) では電波法的にグレーなので、**WiFi/BT を一切 init しない設計**。
ESP32-P4 ↔ 2BP は物理 UART 配線で代替する。詳細: [#5 技適記事](https://yasu-home.com/?p=37)。

### NDA 配下のファイル

`dragon-radar/firmware/2bp-config/` 以下は **Murata NDA 配下** (公式 PDF、NXP UWBIOT SDK、工場 FW バックアップ等)。
gitignore で完全除外、Git に絶対コミットしない。`.gitignore` で `*` + `!.gitignore` パターン。

`my.murata.com` から取得した PDF/SDK は配布禁止。これらに依存する手順は本 README とブログでは技術概念のみ記述し、ソースコード抜粋やキャリブレーション値は載せない。

### UCI 世代揃え

異なる NXP UWB チップ (SR150 / SR040) を組ませる時は SDK の UCI 仕様世代を揃える。
NTF の session ID フィールドが見た目同じでも、v1.x (ID) と v2.0 (Handle) で意味が違う。
詳細: [#3 UCI 世代差記事](https://yasu-home.com/?p=35)。

---

## Related reading

開発過程の知見をブログ記事に分割公開している (https://yasu-home.com 「IoT・組込み」カテゴリ):

| # | 記事 | URL |
|---|---|---|
| 0 | 連載 #0 ドラゴンレーダーを真面目に作る | [https://yasu-home.com/?p=38](https://yasu-home.com/?p=38) |
| 1 | dk6prog (SPSDK) で QN9090 を USB ISP だけで焼く | [https://yasu-home.com/?p=33](https://yasu-home.com/?p=33) |
| 2 | MCUXpresso IDE の headless build で組込ファームを 10 秒で回す | [https://yasu-home.com/?p=34](https://yasu-home.com/?p=34) |
| 3 | UWB ranging が動かない真犯人は「UCI 世代差」だった話 | [https://yasu-home.com/?p=35](https://yasu-home.com/?p=35) |
| 4 | ESP32-P4 × LVGL で「バンダイ ドラゴンレーダー」風 UI を本気で再現する | [https://yasu-home.com/?p=36](https://yasu-home.com/?p=36) |
| 5 | 技適未確認の WiFi ボードを国内コンテストに出すなら何を捨てるか | [https://yasu-home.com/?p=37](https://yasu-home.com/?p=37) |

---

## ライセンス

本リポジトリ内のオリジナルコード (主に `dragon-radar/firmware/esp32-p4/`) はライセンス未確定。
`dragon-radar/firmware/2bp-config/` 以下は NXP / Murata の NDA 配下で再配布不可。
最終的に MIT または Apache-2.0 で公開予定 (応募完了後)。
