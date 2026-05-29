# Dragon Radar UWB - 開発指示書 (Path C / 二段構え版)

DigiKey Make ONE Challenge 2026 応募作品。Murata UWB EVK + Waveshare ESP32-S3-Touch-LCD-1.85 でドラゴンレーダー風の宝探しデバイスを作る。子供と公園で 2-7 個のタグを探し、全部集めると神龍が召喚されてウーロンが「ギャルのパンティおくれーっ」と叫ぶ。

応募締切: **2026-06-22 23:59** (残り **42 日** / 起算 2026-05-11)
GW (4/29-5/6) 終了、平日モード移行。子守再開済で開発時間は **夜 + 週末**。
本ドキュメントは Claude Code への開発指示書として使用する。

---

## 採用方針: **Path C** (QN9090 ファーム書換え)

### Path 比較サマリ (詳細別途検討済)

| Path | 概要 | 採否 | 理由 |
|---|---|---|---|
| Path A | Murata プリビルド FW のまま | ❌ | 2DK が Initiator 固定でタグにできない |
| Path B | QN9090 erase + ESP32 が SR150 を SPI 直叩き | ❌ (今回) | 工数大、2BP 1個で破壊リスク高 |
| **Path C** | **QN9090 にカスタム FW、ESP32 ↔ QN9090 は UART** | ✅ **採用** | 工数中、ハード改造ゼロ、戻せる |

### 二段構え戦略

```
[Stage 1] 5月中: Path C で実装 → 6/22 応募 (実演 2 タグ + 演出で 7 タグ)
[Stage 2] 7-8月: Path B (ESP32 で SR150 を直接制御) に移植チャレンジ
[Stage 3] 9/5-6: Maker Faire Tokyo 2026 出展 (進化版)
```

---

## ハードウェア構成

### 現有機材 (2026-05-03 時点)

| 役割 | 型番 | 数 | 状態 / 備考 |
|------|------|---|------|
| プレイヤー機 UI (旧) | Waveshare ESP32-S3-Touch-LCD-1.85 | 1 | ESP32-S3 + 1.85" 丸型 LCD (360×360, ST77916)。画面が小さく、Bandai 玩具感は出るが「ガチ路線」のため格上げ予定 |
| **プレイヤー機 UI (新) ⭐** | **Waveshare ESP32-P4-WIFI6-Touch-LCD-3.4C** | **1 (発注予定)** | **ESP32-P4 + ESP32-C6 (Wi-Fi6/BT5、未使用)、3.4" 丸型 LCD (800×800, MIPI-DSI 2-lane)、10 点タッチ、デュアルマイク、microSD。$80-90、DigiKey 発注予定** |
| プレイヤー機 UWB | Murata Type 2BP EVK (LBUA0VG2BP-EVK-P) | 1 | NXP **SR150** (距離+AoA可) + QN9090 MCU。Path C では QN9090 にカスタム FW を焼く |
| タグ #1, #2 | Murata Type 2DK EVK (LBUA2ZZ2DK-EVK) | 2 | NXP **SR040** (距離のみ) + QN9090 MCU。CR2032 駆動可、隠して放置できる。Path C で Responder 化 |
| デバッガ/SWD 書込 | NXP MCU-Link Pro | 1 | QN9090 へのカスタム FW 書込用、Bridge UART/Target Power 機能あり |
| 用途未確定 | EA-WO1391-1228 (Embedded Artists 系?) | 1 | Phase 0 で正体特定すること |

### UI 操作系の仕様 (確定)

**物理ボタン × 1 + タッチスクリーン + 音声トリガ の三段構え**:

| 操作系 | 用途 | 実装時期 |
|---|---|---|
| **物理ボタン (MKBKLLJY ø12mm メタル + 緑 LED、配線済)** | スキャン開始 / 一時停止 (アニメリスペクト) + 神龍 LED 演出 | Phase 4-5 |
| **タッチスクリーン (10 点)** | ズーム、タグ情報表示、設定メニュー | Phase 2-3 |
| **音声トリガ (デュアルマイク)** | 「ドラゴンボールを探せ!」で起動 | Phase 7 (Maker Faire 用) |

筐体上部に **MKBKLLJY 緑 LED モメンタリ ボタン (ø12mm)** を配置 (Bandai 玩具と同位置)。
押下時に「ピッ」効果音 (VOICEVOX 生成)、待機中は LED 呼吸光、神龍召喚時は LED 高速点滅。
LED は 12V 仕様なので、ESP32 GPIO から MOSFET で 5V (USB-C 5V) を ON/OFF する構成。

### 機材についての追記
- 2BP EVK のヘッダピン (10ピン×2列) が一部曲がっているが、**Path C では USB ケーブルのみで動作するため影響なし**
- 2BP EVK は **1 個しか保有しない方針** (高価なため、破損時のみ追加発注)
- 2DK EVK の追加 5 個は **6/22 応募後 (7月頃) に発注予定** (Maker Faire 7 タグ拡張用)
- **プレイヤー機 LCD は 1.85" → 3.4" に格上げ決定** (動画映え重視、ESP32-S3 → ESP32-P4 への移行が必要):
  - 旧 1.85" (ESP32-S3-Touch-LCD-1.85) は予備 / Phase 2 検証用に保管
  - 新 3.4" (ESP32-P4-WIFI6-Touch-LCD-3.4C) は 5/4 発注、5/12 頃着想定
  - 筐体: ø ~110mm × 厚 30mm (両手持ち or 大人片手、Bandai より大きいが解像度 800×800 で迫力)
  - 移行作業: ESP-IDF target esp32s3→esp32p4、display_init.c (SPI ST77916 → MIPI-DSI)、LVGL 寸法定数 (360 → 800)、半日〜1日工数

### 9月 Maker Faire 用追加発注 (7月予定)

| 部品 | 数 | 概算 |
|---|---|---|
| 2DK EVK 追加 | 5 | ~$450 |
| (オプション) 自作タグ PCB 部品一式 | 1 セット | ~$300 |

---

## システムアーキテクチャ (Path C)

```
[ プレイヤー機 (筐体内部) ]
┌─────────────────────────────────────────────┐
│ Waveshare ESP32-S3-Touch-LCD-1.85          │
│  ├─ LVGL UI (丸型レーダー)                  │
│  ├─ ゲームステートマシン                    │
│  ├─ 音声再生 (I2S)                         │
│  ├─ microSD (asset)                         │
│  └─ UART パーサ (シンプル ASCII)            │
└──────┬──────────────────────────────────────┘
       │ UART or USB-CDC
       │ "RANGE" → "OK TAG1:1234,30,−5 TAG2:2100,−15,40"
       ▼
┌─────────────────────────────────────────────┐
│ Murata Type 2BP EVK                         │
│  ├─ QN9090 MCU ← カスタム FW (我々が書く)   │
│  │   - NXP UWBIOT SDK ベース               │
│  │   - UCI Host stack (NXP 提供)           │
│  │   - 自作 ASCII コマンド I/F             │
│  │   - 1:N (round-robin) ranging           │
│  └─ SR150 UWB IC (Murata 校正済)           │
└─────────────────────────────────────────────┘
                ↑ UWB 電波
                │
[ タグ × 2 (公園に隠す)、各々独立動作 ]
┌──────────────────────┐  ┌──────────────────────┐
│ Murata Type 2DK EVK  │  │ Murata Type 2DK EVK  │
│  ├─ QN9090 ← Responder FW │  │  ├─ QN9090 ← Responder FW │
│  └─ SR040 UWB IC     │  │  └─ SR040 UWB IC     │
│  ├─ CR2032 電池駆動   │  │  ├─ CR2032 電池駆動   │
└──────────────────────┘  └──────────────────────┘

[ 開発・デバッグ時のみ ]
┌─────────────────────────────────────────────┐
│ MCU-Link Pro (NXP)                          │
│  └─ SWD で QN9090 にファーム書込・デバッグ  │
└─────────────────────────────────────────────┘
```

### データフロー

1. ESP32 が QN9090 に "RANGE" コマンド送信 (33ms 間隔, 約 30Hz)
2. QN9090 が **round-robin で 2 タグ (将来 7) を順番に Unicast TWR 実行**
3. 各タグの距離 (mm) + 方位角 (deg) を集計
4. QN9090 が ESP32 に "OK TAG1:1234,30 TAG2:2100,-15" のような ASCII レスポンス
5. ESP32 がパース → LVGL レーダーに光点プロット
6. ゲームロジックが「30cm 以内に N 秒滞在 = 取得」を判定
7. **2 個取得時点** で 7 個取得演出に分岐 (動画用) or **7 個取得** (Maker Faire 拡張時)
8. 神龍召喚演出 + ウーロン音声 + パンティひらひらアニメ

---

## ディレクトリ構成

```
dragon-radar/
├── README.md
├── firmware/
│   ├── esp32-s3/                # ESP-IDF プロジェクト (UI + ゲーム + UART parser)
│   │   ├── main/
│   │   │   ├── main.c
│   │   │   ├── display_init.c   # ST77916 + LVGL 初期化 (Waveshare driver 取込予定)
│   │   │   ├── ui/
│   │   │   │   ├── radar_view.c/h     # LVGL レーダー UI ✅ 5/3 実装済
│   │   │   │   ├── summon_view.c/h    # 神龍召喚演出
│   │   │   │   └── theme.c/h           # 色・フォント定義
│   │   │   ├── uwb/
│   │   │   │   ├── uwb_uart.c/h        # QN9090 への UART コマンド I/F
│   │   │   │   └── uwb_filter.c/h      # 移動平均・異常値除去
│   │   │   ├── game/
│   │   │   │   └── game_state.c/h
│   │   │   ├── audio/
│   │   │   │   └── audio_player.c/h    # I2S 再生
│   │   │   └── storage/
│   │   │       └── sd_card.c/h
│   │   ├── components/                 # ST77916 driver 等
│   │   ├── CMakeLists.txt
│   │   ├── sdkconfig.defaults
│   │   └── partitions.csv
│   ├── 2bp-fw/                  # ★ Path C で新設: 2BP の QN9090 用カスタム FW
│   │   ├── README.md            # MCUXpresso での開発手順
│   │   ├── src/
│   │   │   ├── main.c                  # FreeRTOS エントリ
│   │   │   ├── uart_cmd.c/h            # ASCII コマンドパーサ
│   │   │   ├── ranging.c/h             # round-robin ranging スケジューラ
│   │   │   └── config.h                # SR150 設定、タグ MAC 一覧
│   │   └── prj/                  # MCUXpresso プロジェクトファイル
│   ├── 2dk-fw/                  # ★ Path C で新設: 2DK の QN9090 用 Responder FW
│   │   ├── README.md
│   │   ├── src/
│   │   │   ├── main.c
│   │   │   └── responder.c/h           # SR040 を Responder として常駐
│   │   └── prj/
│   └── 2bp-config/              # Murata 公式ドキュメント (NDA, gitignore)
│       └── docs/                # 取得済 Murata PDF 25 件
├── assets/
│   ├── audio/                          # VOICEVOX で事前生成
│   │   ├── search_beep.wav
│   │   ├── found.wav
│   │   ├── shenron_appear.wav
│   │   ├── oolong.wav
│   │   └── shenron_grant.wav
│   └── images/
│       ├── star.png
│       ├── shenron.png
│       └── panty.png
├── tools/
│   ├── uart_sniffer.py                  # PC で QN9090 UART を眺めるツール ✅ 5/3 実装済
│   └── captures/                        # raw キャプチャログ (gitignore)
└── docs/
    ├── 2bp-protocol.md                  # 2BP↔ESP32 ASCII プロトコル仕様
    ├── path-c-architecture.md           # Path C アーキテクチャ詳細
    ├── demo-scenario.md                 # 動画台本
    └── parts-list.md                    # myLists 用部品表
```

---

## 開発スケジュール (Path C / 二段構え)

### 現状 (2026-05-11 時点) ✅ 達成済み

- ESP-IDF v5.3.2 インストール (esp32p4 ターゲット対応確認済)
- ARM GCC 13.2 + J-Link Commander インストール
- `dragon-radar/` プロジェクト初期化
- ESP-IDF プロジェクトスケルトン (LVGL 9.2 取込、ビルド成功) - 旧 1.85" 用
- **Phase 2 (LVGL レーダー UI)** 実装済: ダミーデータで光点が動く
- Murata PDF 25 件取得 (NDA 配下、gitignore)
- 2BP/2DK Quick Start Guide 解析、**Path C 採用決定**
- **NXP UWBIOT SR150 v04.08.01 MCUx SDK 取得・展開済** (NDA 配下)
- **Waveshare ESP32-P4-WIFI6-Touch-LCD-3.4C 5/11 着** (本体 + 大型スピーカー)
- ESP32-S3 → ESP32-P4 移行方針確定

## 5/19 Phase 1 完了報告 + Phase 2 引継ぎ

**完了済 (5/19)**

| 項目 | 状態 | 場所 |
|---|---|---|
| ESP-IDF v5.5.4 + esp32p4 ターゲット | ✅ | `~/esp/esp-idf-v5.5` |
| ARM GCC 13.2, J-Link Commander | ✅ | `~/tools/`, `/opt/SEGGER` |
| MCUXpresso IDE for Linux | ✅ | `/usr/local/mcuxpressoide-25.6.136` |
| SPSDK dk6prog (USB ISP 書込) | ✅ | `pip install spsdk` |
| ESP32-P4-WIFI6-Touch-LCD-3.4C | ✅ 動作確認済 (Bandai 風 UI) | `firmware/esp32-p4/` |
| 2BP/2DK ×2 出荷時 FW バックアップ | ✅ 3 ファイル | `firmware/2bp-config/firmware-backups/` (gitignore) |
| 2BP に 2bp_controller_v04.08.01.bin 書込 | ✅ dk6prog 経由 | - |
| 2DK ×2 に 2dk_controlee_v04.03.14.bin 書込 | ✅ dk6prog 経由 | - |
| Murata PnP script 取得 (SR150 + SR040) | ✅ | `firmware/2bp-config/sdk/MTD-SCP-*` |

**観測値 (5/19 実機計測)**

- 2BP RANGE_DATA_NTF 内の Session 識別子: `0x00000001` (Session Handle と推測)
- 2DK RANGE_DATA_NTF 内の Session 識別子: `0x11223344` (Session ID そのまま)
- → 一見「Session ID 不一致」に見えるが、SDK source 確認結果 **両方とも内部設定は `0x11223344`** で一致
  (`demos/SR1XX/demo_ranging_controller/demo_ranging_controller.c:31` および
   `demos/SR040/demo_tracker_sr040/app_Ranging_Cfg.h:17` で定義)
- → 実際の原因は **UCI プロトコル仕様の世代差** (v04.08.01 は UCI 2.0+ で Session Handle 採用、
  v04.03.14 は旧 UCI で Session ID 直接表記) と推定
- → **2BP/2DK の UCI バージョンを揃える** ことが ranging 完成の鍵
- Murata 2bp_prebuild パッチは session ID を変更しない (TX_POWER + XTAL_CAP 校正と ranging timeout 5→30 分のみ)
- 2DK は SR040 へ毎起動で SWUP (Software Update) push が必要 (host script が ~30 秒で 275 component を転送)
- FTDI 工場シリアル: 全 EVK 同じ `DM86TTWC` (個体識別性なし、bus address で区別)
- dk6prog PYFTDI backend は **要 sudo** (USB raw access のため)
- dk6prog の引数: `-d DM86TTWC` で接続、`read/erase/write -o file 0x0 0x9DE00` で操作

**機材コンディション (5/19 時点)**

- 2BP の SR150 SPI ブレークアウトピン (TP20/TP81-87 周辺、2x4 ヘッダ): 一部曲がり/欠損
  → Path C では使わないので**実害なし**
- 2BP の QN9090 SWD ピン: **状態不明** (Quick Start 図と一致するコネクタを未特定)
  → USB ISP モード (dk6prog) で代替できているので、現状問題なし
- 2DK の SWD ピン: **生存**
  → 必要なら J-Link でデバッグ可能、Phase 2 開発時の保険として有用
- USB-A to Micro-B データ通信対応ケーブル: 必須 (充電専用品は不可)

**Phase 2 への引継ぎ事項**

1. **session ID 統一** が ranging 完成の鍵
   - 自前ビルド時に `Demo_Common_Config.c` の `SESSION_ID` を 2BP/2DK 両者で揃える
   - 推奨: `0x00112233` のような固定値、両側のソースを編集して再ビルド
2. **NXP UWBIOT SDK で MCUXpresso 上でビルド** (PDF: `Type 2BP How to Build Pre-Built Binary.pdf` 参照)
   - 2BP: SR150 SDK v04.08.01 のソース + Murata パッチ `2bp_prebuild_v04.08.01.patch`
   - 2DK: SR040 SDK v04.03.14 のソース + Murata パッチ `2dk_prebuilt_v04.03.14.patch`
3. **ESP32 ↔ QN9090 ASCII プロトコル** 実装 ([docs/2bp-protocol.md](dragon-radar/docs/2bp-protocol.md) 参照)
4. 校正値は 2BP/2DK 個体ごとに記録 (`UWB_DeviceConfig_SR1XX.h`)、Murata パッチで適用される
5. 万一書込ミスったら **firmware-backups/** から `dk6prog write 0x0 <backup>.bin` で原状復帰可能

**Phase 1 検証で得た知見 (将来トラブルシュート用)**

| 症状 | 原因/対処 |
|---|---|
| `Sem Timed out` / `UwbApi_Init Failed` | SR040 への SWUP 未完了、or SR150 と SR040 の SDK バージョン不整合 |
| `UCI Header is not valid` 連発 | QN9090 がまだ ISP モード (リセット必要) / FW バージョン不一致 |
| `2 USB devices match URL` | dk6prog は同シリアル 2 台同時不可、片方を物理切断 |
| `--enable-non-contiguous-regions discards section ... 21KB` | ESP32-P4 v1.x シリコンで `CONFIG_SPIRAM_XIP_FROM_PSRAM` 有効だと IRAM 溢れ → 無効化 |
| 起動 garbage 出力で baud 合わない | ISP モードから抜けてない、リセットボタンで物理リセット必要 |

---

## 5/19 Phase 2 ブレイクスルー: 2BP↔2DK Ranging 成立 🐉

**結論**: UCI バージョンを揃えれば **そのまま ranging 通る**。session ID の問題は副次的だった。

**実施した組合せ**

| 役割 | EVK | 書込 FW | UCI ver | 出所 |
|---|---|---|---|---|
| Initiator/Controller | 2BP (SR150 + QN9090) | `demo_ranging_controller-SR150-v04.04.03.bin` | **1.31** | NXP UWBIOT SDK v04.04.03 (Rhodes4_SE 用 bare bin) |
| Responder/Controlee | 2DK (SR040 + QN9090) | `2dk_controlee_v04.03.14.bin` | **1.31** | Murata Standalone v04.03.14 (校正済) |

これまでの v04.08.01 (UCI 2.0) ↔ v04.03.14 (UCI 1.31) では Session Handle/Session ID の表現が違って pair 不成立だったが、**両端を UCI 1.31 に揃えた瞬間に成立**した。

**観測結果 (50–60 秒間の同時計測 / 手元配置 30–50 cm)**

| EVK | サンプル数 | min | max | avg |
|---|---:|---:|---:|---:|
| 2BP (SR150 initiator) | 242 | 24 cm | 55 cm | **36.1 cm** |
| 2DK (SR040 responder) | 296 | 22 cm | 54 cm | **35.6 cm** |

両端で平均値の差 **0.5 cm**、レンジも合致。実距離 (手元 30–50 cm) と整合し、SR040 側で校正済 (Murata パッチ) の効果と思われる。

**この組合せの欠点と次の手**

- 2BP 側は **NXP 公式 Rhodes4_SE バイナリ** を流用しているため、Murata の TX_POWER / XTAL_CAP / 周辺校正が未適用 → 絶対距離精度は妥協ライン。
- AoA はまだ未確認 (TWR distance のみ)。`RANGE_DATA_NTF` の AoA フィールドはパース必要。
- Phase 2 で **Murata SDK v04.04.03 ベースの自前ビルド + 校正値適用** を行えば理想形。
- session ID は両ファームともデフォルトで `0x11223344` 採用なので、自前ビルドでも変更不要。

**意味**

- Phase C のロジックレベルでは「2BP=Initiator、2DK=Responder、両者 UCI 1.31、session=`0x11223344`」が黄金パターン。
- Phase 2 以降 (カスタム QN9090 FW + ESP32 ASCII プロトコル) のターゲット FW バージョンは **NXP SDK v04.04.03 / UCI 1.31** に確定。

---

## Phase 2 着手方針 (5/19 確定)

### ベース SDK: v04.04.03 / Murata 校正なし

| 項目 | 値 |
|---|---|
| ベース SDK | NXP UWBIOT SDK v04.04.03 (`UWBIOT_SR150_v04.04.03_MCUx/uwbiot-top/`) |
| MCUXpresso project | `project/RhodesV4_SE/` (`.project` + `.cproject` あり) |
| Demo | `demos/SR1XX/demo_ranging_controller/demo_ranging_controller.c` |
| Murata 校正 | **適用しない** (v04.04.03 用パッチが Murata から未配布、v04.06.05 用パッチは UCI opcode `2E11→2F21` 差で直接適用不可)。絶対距離精度は妥協、レーダー演出に必要な相対変化は十分得られる。 |
| ビルド方式 | MCUXpresso IDE GUI もしくは `mcuxpressoide -application org.eclipse.cdt.managedbuilder.core.headlessbuild` (CDT headless) |
| 書込 | dk6prog (USB ISP) — Phase 1 と同じ |

### Phase 2 サブタスク

- **2a**. SDK 同梱の `demo_ranging_controller` を **無改変でビルド**。`binaries/Rhodes4_SE/demo_ranging_controller-SR150-v04.04.03.bin` (既に動作確認済の bare bin) と sha 一致を確認 → ビルド環境健全性のチェック。
- **2b**. `libs/uwb-iot/uwb_api/PrintUtility/PrintUtility.c` の `printRangingData()` 内で `NXPLOG_APP_D("TWR[...].aoa_*")` 系を **カスタム ASCII フォーマットに置換**。
  - 提案フォーマット: `RADAR,t=<tag_id>,d=<dist_cm>,az=<az_q9.7>,el=<el_q9.7>,st=<status>\n`
  - APP 層 (NXPLOG_APP_I) で出すことで、既存の `TWR[0].distance` printf と同じ UART/baud で取得可能
- **2c**. 改修バイナリを 2BP に dk6prog で焼く。2DK は v04.03.14 のまま。Murata Python script ではなく ESP32 (もしくは PC ホストテスト) で UART を直 read。
- **2d**. ESP32-P4 側で UART (現状 GPIO 未確定) を初期化し、`RADAR,...` を parse → LVGL `radar_view.c` の光点座標に変換 (極座標 → 直交変換)。

### ASCII プロトコル設計案

ESP32 ← QN9090 (notifier):

```
RADAR,t=0,d=42,az=-15.3,el=2.1,st=0\n     # tag 0 ranging OK
TICK,uptime=12345\n                        # 1Hz heartbeat
LOG,msg=session_started\n                  # 任意ログ
```

ESP32 → QN9090 (control, Phase 2c+ で実装):

```
START\n                                     # ranging 開始
STOP\n                                      # ranging 停止
SET_TAG,id=1,mac=2223\n                    # 7 タグ対応の追加 (Phase B)
```

詳細プロトコル仕様は [docs/2bp-protocol.md](dragon-radar/docs/2bp-protocol.md) に切り出し済み。

### リスクと回避策

| リスク | 回避策 |
|---|---|
| MCUXpresso headless build がうまく動かない | GUI で代替、もしくは `mcuxpressoide-ide -nosplash` の `-application` 指定で試す |
| Build 成果物 bin と Rhodes4_SE 同梱 bin の sha が一致しない | 元の binaries/ にある bin はメーカー署名済の可能性 → 違って当然と見て、動作で確認 |
| AoA Azimuth が 0 で返ってくる | SR150 のアンテナ delay calib 未適用が原因の可能性 → 値が出ていれば校正ズレ程度なので進める |
| `printRangingData` 改修で他デモがビルド不能になる | `#ifdef UWBIOT_APP_BUILD__DEMO_RANGING_CONTROLLER` で囲って影響範囲を限定 |

### Phase 2 実機検証ログ (5/19 16:10)

| 項目 | 値 |
|---|---|
| 2BP FW | `firmware-builds/2bp_dragon_radar_v0.1.bin` (353312 B、v04.04.03 SDK Debug ビルド) |
| 2DK FW | `2dk_controlee_v04.03.14.bin` (出荷時 Murata 校正済) |
| UART baud (debug console) | **3 000 000 bps** (`boards/Rhodes4_SPI/board.h:203`) |
| ranging rate | ~5 Hz (20 秒で 100 行) |
| 距離 (手元配置) | 19–24 cm |
| AoA Azimuth | -44° 〜 -45° (左方向に 2DK、安定) |
| AoA Elevation | -60.0° に固定 (SR150 は方位アンテナのみ、仰角は無効) |
| サンプルログ | `firmware-builds/sample_radar_uart_20s.bin` (35613 B) |

**出力フォーマット例** (3Mbps UART、ANSI色付き):

```
APP     :INFO :TWR[0].distance        : 21
APP     :INFO :RADAR,t=0,d=21,az=-44.18,el=-60.0,st=0
```

ESP32 側パース仕様 (Phase 2d で実装):
- baud: 3000000、ANSI escape (`\e[0;34m`〜`\e[0m`) は除去
- `RADAR,t=<tag>,d=<cm>,az=<int>.<frac>,el=<int>.<frac>,st=<status>` を抽出
- 角度: Q9.7 表記、`real_deg = int_part + (frac/128.0)` (符号は int_part に従う) で復元
- status==0 のみ採用、他は drop
- ranging timeout = 5 分なので再起動時はリセット必要

### Phase 2d-5 完了 (5/19 夜、実機 + LCD 確認済) ✅

ESP32-P4 GPIO47 ← 2BP TP48 の物理配線で、LCD に光点が出ることを確認。
全経路成立: `2DK ─UWB→ 2BP/SR150 → QN9090(RADAR ASCII) → UART線 → ESP32 GPIO47 → parser → radar_view → LCD 光点`。

**実装時に踏んだバグ 2 件 (修正済)**

1. **`radar_view_set_tag()` が `tag_id == 0` を早期 return で drop**
   - QN9090 は `RADAR,t=0` (UWB measurement index は 0-based) を送るが、radar_view は tag_id を 1-based (`s_dots[tag_id-1]`) で扱う設計
   - 修正: parser (`uwb_uart.c`) で `tag_id = t + 1` に変換
2. **`app_main` を return させると表示が消える**
   - この BSP/esp_lvgl_adapter 構成では app_main が return すると static UI ごと消える
   - 修正: app_main 末尾に `while(1) vTaskDelay` を置いて生かし続ける (Phase 1 にあったものを誤って削除していた)

**運用上の注意**

- ~~2BP のカスタム FW は起動後 **5 分で ranging セッションを閉じて停止**~~ → **v0.2 で解消** (下記)。
- 旧 v0.1 の再ペア手順 (参考): ① 2DK を先にリセット → ②数秒後に 2BP をリセット。

**5/29: QN9090 FW v0.2 (連続 ranging 化)**

- `demo_ranging_controller.c` の `delay = 5*60*1000` を `delay = 0xFFFFFFFFUL` に変更 (~49 日 = 実質無限)。
- セッションを張りっぱなしにすることで、**2DK が一旦切れて戻っても自動で再 join** する → 5分制限と手動再ペアの両方を解消。
- ビルド済: `firmware-builds/2bp_dragon_radar_v0.2_continuous.bin` (353304 B)。

**⚠️ ISP 書込時は ESP32↔2BP の配線を外すこと (重要)**

- ESP32 の **GPIO48 (UART TX) → 2BP TP47 (QN9090 PIO_9/USART0_RXD)** が繋がっていると、dk6prog の ISP ハンドシェイク (FTDI が PIO_9 経由でコマンド送信) と**競合して timeout**する。
- 症状: `dk6prog info` が `TimeoutError`、Chip ID が返らない。
- 対処: 書込前に最低限 **TP47↔GPIO48 線を外す** (安全のため 3 本とも外すか ESP32 電源 OFF)。書込後に戻す。
- v0.1 書込時に問題なかったのは、当時まだ配線していなかったため。

### 技適制約 (5/19 確定)

Waveshare ESP32-P4-WIFI6-Touch-LCD-3.4C 基板上の ESP32-C6 モジュールに **技適マーク未確認**。
公開展示 (DigiKey Make ONE Challenge) では電波法違反になり得るため、

**本プロジェクトでは ESP32-P4 ボードの WiFi/BT を一切使用しない方針**。

影響:
- 2BP ↔ ESP32-P4 通信は **物理配線 (UART 直結) で固定** (A2 ルート確定)
- 将来の拡張案 (BLE スマホ連携、WiFi 経由ファーム更新) もこのボードで実現する場合は技適認証品への基板差替を要する
- LCD / Touch / microSD / GPIO など WiFi 以外の機能は使用 OK

### 配線計画 (5/19 EVK Rev4.1 Schematic + Waveshare 3.4C ボード写真確認済)

| 端点 (2BP) | 信号 | 電圧 | 端点 (ESP32-P4) | 補足 |
|---|---|---|---|---|
| **TP48** | QN9090 PIO_8 / USART0_TXD | 3.3V CMOS | **GPIO 47** (UART1 RX) | 主信号 (RADAR 3Mbps) |
| **TP47** | QN9090 PIO_9 / USART0_RXD | 3.3V CMOS | **GPIO 48** (UART1 TX) | optional, 制御送信用 |
| GND (TP29 等) | GND | — | GND | 必須 |

**配線番号の罠**: 2BP 側の "TP48" と ESP32-P4 側の "GPIO 48" は別物 (TP48 ↔ GPIO 47 がペア、TP47 ↔ GPIO 48 がペア)。番号が交差するので識別ミスに注意。

ESP32-P4 GPIO 47/48 を選んだ根拠:
- Waveshare 3.4C の 40 ピン拡張ヘッダ右列で**物理的に隣接**(配線楽)
- BSP (LCD/Touch/I2S/SD) が使う GPIO (4,6-13,26,27,32,39-44,53) と衝突しない
- ESP32-P4 GPIO matrix で UART1 を任意 GPIO に割当可能

備考:
- 以前 Phase 1 で「曲がっていた」TP20 / TP81-87 は SR150 SPI ブレークアウト用 (Path B 想定)、**TP47/TP48 とは別位置で健在**
- baud = 3 Mbps、QN9090 側は ANSI 色付き ASCII で出力
- TP47 への送信は Phase 2 後段 (制御プロトコル) で必要、Phase 2 初回は RX のみで OK
- SWD ピン (TP40/TP41) は QN9090 デバッグ用、ISP_ENTRY (TP36) は dk6prog 書込時のリセット用

---

### Phase 0: 環境準備 + 校正値バックアップ 【5/11-12】 (大半済)

**目的**: Path C の安全マージン確保。失敗時に元の Murata プリビルド FW に戻せる準備。

**タスク**

1. NXP MyAccount 登録 (まだなら)
2. NXP UWBIOT SDK ダウンロード (SR150 v04.06.00 推奨)
3. MCUXpresso IDE インストール (QN9090 開発用)
4. MCU-Link Pro のドライバ確認、PC で認識
5. **2BP/2DK 各 EVK の QN9090 フラッシュ全領域をダンプ**
   - `LinkServer flash -p QN9030 read 0x00000000 0x80000 backup_<EVK名>_qn9090.bin`
   - 校正領域 (IFR) も別途ダンプ
   - ダンプファイルは `firmware/2bp-config/firmware-backups/` (gitignore) に保管
6. EA-WO1391-1228 の正体確認 (実物見て M.2 か等)

**成果物**: `firmware-backups/2bp.bin`, `2dk_1.bin`, `2dk_2.bin`

**詰まりポイント**: NXP MyAccount は登録に時間かかることあり、早めに着手

### Phase 1: QN9090 で Murata サンプル動作確認 【5/5-12】

**目的**: NXP SDK でビルド・書込・動作確認のサイクルを確立。

**タスク**

1. NXP UWBIOT SDK の `demos/SR1XX/demo_ranging_controller` をビルド
2. Murata 提供のパッチ (`2bp_prebuilt_*.patch`) を適用
3. MCU-Link Pro 経由で 2BP に焼く
4. PC で UART モニタしながら Initiator 動作確認 (ペアの 2DK は出荷時 FW のまま)
5. 同様に 2DK に Responder ベースのサンプルを焼く試行 (Initiator サンプルを改造)

**成果物**: 自分でビルドした FW で 1:1 ranging が動く

### Phase 2: カスタム ASCII プロトコル実装 【5/12-19】

**目的**: ESP32 ↔ 2BP の自前 UART プロトコル定義・実装。

**タスク**

1. `firmware/2bp-fw/` にプロジェクト雛形作成
2. ASCII コマンド仕様策定 (`docs/2bp-protocol.md` 確定):
   - `RANGE` → `OK TAG1:dist,az,el TAG2:dist,az,el ...`
   - `SET_TAGS <count> <mac1> <mac2> ...`
   - `STATUS` → `READY` / `BUSY` / `ERROR`
3. UART タスクと Ranging タスクを FreeRTOS で分離
4. round-robin スケジューラ (タグ N 個を順番に Unicast TWR)
5. ESP32 側 `main/uwb/uwb_uart.c` で ASCII パース実装
6. 1:1 で実距離・方位データが LVGL レーダーに乗る

**成果物**: ESP32 のレーダー画面に 1 タグの実距離が出る

### Phase 3: 2DK Responder 化 + 2 タグ同時 【5/19-26】

**目的**: 2 タグを round-robin で同時に追跡。

**タスク**

1. `firmware/2dk-fw/` で 2DK 用 Responder FW 実装
2. 2DK ×2 個を異なる UWB MAC で Responder として常駐
3. 2BP の round-robin スケジューラを 2 タグ対応化
4. ESP32 のレーダー画面に 2 タグの実位置が出る
5. ゲームロジック (30cm 以内 N秒で取得判定) の実装・チューニング

**成果物**: 2 タグを近づけたら順次「取得」アニメ → 星が増える

### Phase 4: 神龍演出 + ウーロン音声 + 7タグ演出 【5/26-6/10】

**目的**: 7 タグ揃った演出を仕上げる (実機 2 タグ + 5 タグぶん演出)。

**タスク**

1. VOICEVOX で WAV 出力:
   - shenron_appear.wav: 「神よ…願いを言え…」
   - oolong.wav: 「ギャルのパンティおくれーっ！」
   - shenron_grant.wav: 「よかろう…」
2. SD カードから WAV を I2S で再生
3. 神龍召喚アニメ (緑→金フェード + 雷 + 龍シルエット)
4. パンティ演出 (PNG が左右に揺れて降ってくる)
5. **「2 個実機取得 → 5 個ぶんは演出のみで足す」分岐ロジック** (動画用)
6. 起動から終了までの状態遷移を一通り通す

**成果物**: 「2 タグ取って → 神龍召喚 → ウーロン → 終了」が一気通貫で動く

### Phase 5: デモ動画撮影 + ProtoPedia 応募 【6/10-22】

**シナリオ**: 詳細は `docs/demo-scenario.md`

| 時間 | 内容 |
|------|------|
| 0:00-0:10 | タイトル: 「Dragon Radar UWB - DigiKey Make ONE Challenge 2026」 |
| 0:10-0:25 | ハード紹介: Murata 2BP/2DK、Waveshare 丸型 LCD のアップ |
| 0:25-0:40 | 「自分イチ」テーマ説明 |
| 0:40-1:20 | **公園で子供がタグ 2 個を探すデモ** (本物の UWB 動作) |
| 1:20-1:30 | カット繋ぎで「7 個目発見」演出 (内部で擬似的に 7 個揃える) |
| 1:30-1:45 | 神龍召喚 |
| 1:45-1:55 | ウーロン降臨「パンティおくれーっ」 |
| 1:55-2:00 | クレジット |

**応募内容**:
- ProtoPedia 一般公開で登録
- デモ動画 URL (YouTube 限定公開可)
- myLists 部品表 URL
- 「自分イチ」テーマ:
  - 候補: 「自分イチ初の UWB 統合」「自分イチ初の MCUXpresso ファーム開発」「自分イチ初の二段 MCU 構成 (ESP32 ↔ QN9090) 通信設計」
- 使用パーツ全 SKU、開発過程の写真、GitHub リンク (MIT or Apache-2.0)

### Phase 6: Path B 移植チャレンジ 【7-8月、応募後】

**目的**: ESP32 で SR150 を SPI 直叩きできるようにする (Maker Faire 用「進化版」)。

**タスク**

1. NXP UWBIOT SDK の HAL 層 (SPI transport, GPIO, timer) を ESP-IDF に移植
2. UCI core を ESP32 上で動作確認
3. 校正値を `UWB_DeviceConfig_SR1XX.h` から移植
4. 既存の Path C 実装と切替可能に (config flag で選択)

### Phase 7: 7 タグ拡張 + Maker Faire 出展 【7-9月】

**タスク**

1. 6/22 応募後すぐ 2DK ×5 を DigiKey に発注
2. 5 個ぶんの 2DK に Responder FW 焼込
3. round-robin スケジューラを 7 タグ対応化、サイクル時間チューニング
4. (オプション) 自作タグ PCB 設計・試作
5. Maker Faire Tokyo 2026 (2026-09-05/06) で 7 タグ完全版を実演

---

## 依存関係

### ESP32-S3 側 (ESP-IDF)

- ESP-IDF v5.3.2 ✅ インストール済 (`~/esp/esp-idf`)
- LVGL v9.2 ✅ managed component で取得済
- ST77916 LCD driver (Waveshare 公式 GitHub から取込予定)
- esp_lcd_touch_cst816s ✅ managed component で取得済
- esp_audio or esp-adf (WAV 再生)
- FATFS (SD カードアクセス)

### QN9090 側 (Path C で新設)

- MCUXpresso IDE
- NXP UWBIOT SDK SR150 v04.06.00 + Murata パッチ
- LinkServer (MCU-Link Pro と通信)

### ホスト側ツール

- VOICEVOX - 音声生成
- ffmpeg - 動画編集 + WAV 変換
- LVGL Image Converter - PNG→C 配列
- DaVinci Resolve / CapCut - 動画編集

---

## トラブルシューティングメモ

| 症状 | 対処 |
|------|------|
| QN9090 認識されない | MCU-Link Pro のファーム最新化、SWD ピン位置確認、Target Power 設定 |
| カスタム FW 焼いた後動かない | `firmware-backups/` から元の Murata FW を復元 |
| LCD が真っ黒 | backlight GPIO HIGH 確認、SPI clock を 10MHz まで下げる、PSRAM 有効化確認 |
| LVGL がカクつく | `LV_COLOR_DEPTH=16`、PSRAM 有効化、frame buffer を internal RAM に |
| UART 文字化け | ボーレート再確認、GND 共通確認、配線長を 10cm 以内に |
| AoA がフラフラ | 移動平均 window を 10 まで増やす、屋外で撮影 |
| WAV が再生されない | I2S ピン番号確認、サンプルレート 16kHz / 16bit / mono か確認 |
| ranging のレートが低い | round-robin の各 TWR を高速化 (UCI session を維持したまま target 切替) |

---

## 重要な制約と判断記録

### Murata FW のロール制約 (重要)

- **2DK の出荷時 FW は Initiator/Controller 固定**。タグ (Responder) として使うにはカスタム FW 必須。
- 2BP は出荷時 FW で Initiator/Responder 両対応だが、PnP モードは Unicast 1:1 のみ。

### サンプルスクリプトはすべて Unicast

Murata 提供の Python スクリプトは `*_Unicast_*.py` のみ。1:N (Multicast) TWR は SDK 側でサポートされているが、サンプルは未提供。Phase 2-3 で round-robin Unicast を自前実装する。

### 2BP 1 個運用のリスクと対策

- 2BP は ~$150 と高価、保有 1 個のみ
- Path C 採用により破壊的操作 (QN9090 erase など) は不要
- それでも Phase 0 で必ず QN9090 フラッシュをバックアップ
- カスタム FW 開発中は MCU-Link Pro 経由で何度でも書き戻し可能

### EA-WO1391-1228 の扱い

- 正体未確定 (Embedded Artists 系の M.2 系モジュール推定)
- Phase 0 で確認、用途が判明したら本仕様書に追記

---

## Claude Code への指示

1. Phase 0 から順番に着手。各 Phase 完了後に成果物を確認してから次へ進む。
2. 各タスクは git commit を細かく切る (Phase ごとに最低 3 コミット)。
3. 完了したタスクはチェックボックスにチェックを入れる。
4. 詰まったら作業をブロックして人間に質問する (推測で進めない)。
5. ESP-IDF / MCUXpresso のビルドエラーは抱え込まずに即報告。
6. ハードウェアに依存する確認 (LCD 表示、UART 受信、QN9090 書込、UWB 動作) は人間が実機で確認、結果をフィードバック。
7. NXP UWBIOT SDK の中身は NDA 配下、コミット禁止 (`firmware/2bp-config/` は gitignore 済)。
8. **Path B 移植は 6/22 応募完了後に着手** (それまでは Path C に集中)。

開発開始日: 2026-04-29
GW 残り集中期間: 5/3 (日) - 5/4 (月)
コンテスト応募締切: **2026-06-22**
Path B 移植開始: 2026-07 (応募後)
Maker Faire Tokyo 2026: 2026-09-05/06
