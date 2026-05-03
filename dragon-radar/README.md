# Dragon Radar UWB

DigiKey Make ONE Challenge 2026 応募作品。Murata UWB EVK + Waveshare ESP32-S3 で「ドラゴンレーダー風」の宝探しデバイスを作る。

詳細仕様は [`../dragon-radar-spec.md`](../dragon-radar-spec.md) を参照。

## 採用方針: Path C (二段構え)

```
Stage 1 (5月中)  : Path C (QN9090 カスタム FW)   → 6/22 ProtoPedia 応募
Stage 2 (7-8月)  : Path B (ESP32 ↔ SR150 SPI 直結) に移植
Stage 3 (9/5-6)  : Maker Faire Tokyo 2026 で 7 タグ完全版を出展
```

## ハードウェア (現有 3 機)

| 役割 | 型番 | 数 |
|---|---|---|
| プレイヤー機 UI | Waveshare ESP32-S3-Touch-LCD-1.85 | 1 |
| プレイヤー機 UWB | Murata 2BP EVK (LBUA0VG2BP-EVK-P) | 1 |
| タグ #1, #2 | Murata 2DK EVK (LBUA2ZZ2DK-EVK) | 2 |
| デバッガ | NXP MCU-Link Pro | 1 |

## システム構成 (Path C)

```
ESP32-S3 ── UART (USB-CDC) ── 2BP/QN9090 (カスタム FW) ── SR150 ── UWB ── 2DK/QN9090 (Responder FW) × 2
└─ LVGL UI                    └─ UCI Host stack
└─ ゲーム / 音声               └─ round-robin scheduler
```

## ディレクトリ

```
dragon-radar/
├── firmware/
│   ├── esp32-s3/      # ESP-IDF (UI + ゲーム + UART parser)
│   ├── 2bp-fw/        # 2BP の QN9090 用カスタム FW (Path C で新設)
│   ├── 2dk-fw/        # 2DK の QN9090 用 Responder FW (Path C で新設)
│   └── 2bp-config/    # Murata 公式 PDF (NDA、gitignore)
├── assets/
│   ├── audio/         # WAV (神龍/ウーロン)
│   └── images/        # PNG (光点/龍/星)
├── tools/             # PC 側スクリプト (UART sniffer 等)
└── docs/              # プロトコル仕様、デモ台本
```

## 開発状況

- [x] 仕様書 ([dragon-radar-spec.md](../dragon-radar-spec.md)) — Path C 版
- [x] プロジェクト初期化 / ESP-IDF v5.3.2 セットアップ
- [x] **Phase 2 ESP32 側**: LVGL レーダー UI (ダミーデータで光点が動く) ✅ 5/3
- [ ] **Phase 0**: NXP UWBIOT SDK + MCUXpresso セットアップ + 校正値バックアップ
- [ ] **Phase 1**: QN9090 で Murata サンプル動作確認
- [ ] **Phase 2 QN9090 側**: ASCII プロトコル実装
- [ ] **Phase 3**: 2DK Responder 化 + 2 タグ同時 ranging
- [ ] **Phase 4**: 神龍演出 + ウーロン音声 + 7タグ演出
- [ ] **Phase 5**: デモ動画撮影 + ProtoPedia 応募 (~6/22)
- [ ] **Phase 6**: Path B (SPI 直結) 移植チャレンジ (7-8月)
- [ ] **Phase 7**: 7 タグ拡張 + Maker Faire 出展 (~9/6)

## ビルド (ESP32-S3 側)

```bash
. ~/esp/esp-idf/export.sh
cd firmware/esp32-s3
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

## ビルド (QN9090 側、Path C)

MCUXpresso IDE をインストール後、`firmware/2bp-fw/prj/` を Import して Build → Debug。
詳細は [`firmware/2bp-fw/README.md`](firmware/2bp-fw/README.md) (Phase 0 で整備予定)。
