# Dragon Radar UWB - 開発指示書

DigiKey Make ONE Challenge 2026 応募作品。村田UWB EVK + Waveshare ESP32-S3-Touch-LCD-1.85 でドラゴンレーダー風の宝探しデバイスを作る。子供と公園で7個のタグを探し、全部集めると神龍が召喚されてウーロンが「ギャルのパンティおくれーっ」と叫ぶ。

応募締切: **2026年6月22日 23:59**
GW集中開発期間: **4/29(水) - 5/4(月)**
本ドキュメントはClaude Codeへの開発指示書として使用する。

---

## ハードウェア構成

| 役割 | 型番 | 数量 | 備考 |
|------|------|------|------|
| メインデバイス | Waveshare ESP32-S3-Touch-LCD-1.85 | 1 | ESP32-S3 + 1.85" 丸型 LCD (360×360, ST77916 driver), 静電容量タッチ, USB-C, microSD, I2S audio |
| UWB マスター | Murata Type 2BP EVK (LBUA0VG2BP-EVK-P) | 1 | QN9090 MCU + UWB, 距離+AoA 計算, UART 出力 |
| UWB タグ | Murata Type 2DK EVK (LBUA2ZZ2DK-EVK) | 2 (→最終7) | 探す対象（ドラゴンボール7個に相当） |

## システムアーキテクチャ

```
[2DK タグ × N] ←UWB→ [2BP マスター/QN9090] ←UART→ [ESP32-S3 / Waveshare] ─┬─ 丸型LCD (LVGL)
                                                                           ├─ I2S Audio (神龍/ウーロン)
                                                                           └─ microSD (WAV/PNG asset)
```

**データフロー**

1. 2DK タグが UWB ビーコンを発信
2. 2BP の QN9090 が Two-Way Ranging で距離 (mm)、Angle of Arrival で方位角・仰角 (deg) を計算
3. 結果を UART で ESP32-S3 に送信 (フォーマットは Phase 1 で確定)
4. ESP32-S3 が LVGL で極座標レーダーに光点プロット
5. ゲームロジックが「30cm 以内に N 秒滞在 = 取得」を判定
6. 7個収集完了で神龍召喚演出 + ウーロン音声 + パンティひらひらアニメ

---

## ディレクトリ構成

```
dragon-radar/
├── README.md
├── firmware/
│   ├── esp32-s3/                # メイン開発対象 (ESP-IDF)
│   │   ├── main/
│   │   │   ├── main.c
│   │   │   ├── ui/              # LVGL レーダー UI
│   │   │   │   ├── radar_view.c/h
│   │   │   │   ├── summon_view.c/h    # 神龍召喚演出
│   │   │   │   └── theme.c/h          # 色・フォント定義
│   │   │   ├── uwb/              # 2BP からの UART 受信
│   │   │   │   ├── uwb_uart.c/h
│   │   │   │   └── uwb_filter.c/h     # 移動平均・異常値除去
│   │   │   ├── game/             # ゲーム状態機械
│   │   │   │   └── game_state.c/h
│   │   │   ├── audio/            # I2S 再生
│   │   │   │   └── audio_player.c/h
│   │   │   └── storage/          # SD カードアクセス
│   │   │       └── sd_card.c/h
│   │   ├── components/           # ST77916 driver 等
│   │   ├── CMakeLists.txt
│   │   ├── sdkconfig.defaults
│   │   └── partitions.csv        # SPIFFS or FATFS for asset
│   └── 2bp-config/               # Murata サンプルファームの動作メモ
│       └── README.md
├── assets/
│   ├── audio/                    # VOICEVOX で事前生成
│   │   ├── search_beep.wav       # ピピピ音
│   │   ├── found.wav             # タグ取得時
│   │   ├── shenron_appear.wav    # 神よ…願いを言え…
│   │   ├── oolong.wav            # ギャルのパンティおくれーっ
│   │   └── shenron_grant.wav     # よかろう…
│   └── images/                   # PNG (LVGL 用、RGB565 変換推奨)
│       ├── star.png              # 集めた数表示用
│       ├── shenron.png           # 神龍シルエット
│       └── panty.png             # 落ちてくる演出用
└── docs/
    ├── demo-scenario.md          # 動画台本
    └── parts-list.md             # myLists 用部品表
```

---

## GW 開発スケジュール (4/29 - 5/4)

### Phase 1: UART 疎通確認 【4/29 (水) 夜 - 4/30 (木)】

**目的**: 2BP からの距離・角度データが ESP32-S3 まで届くことを確認する。ここで詰まると全工程が止まる最重要フェーズ。

**タスク**

1. 2BP の出荷時 demo firmware のドキュメントを Murata の開発者ポータル (https://www.murata.com/products/connectivitymodule/uwb) で確認
2. 2BP のデバッグ UART ピン (おそらく Arduino ヘッダ上の D0/D1 or 専用ヘッダ) を ESP32-S3 の任意 GPIO に配線
   - 2BP TX → ESP32 RX (GPIO はピン衝突を確認、UART2 推奨)
   - 2BP RX → ESP32 TX
   - **GND 共通必須**
3. ESP32-S3 で UART2 を初期化 (115200 8N1 で開始、ダメなら 9600 / 921600 を試す)
4. まず raw bytes をシリアルモニタにダンプ
5. 2DK タグ電源 ON でデータが流れることを確認
6. データ形式 (Murata 標準は MAUI = Murata Application UART Interface or NTB シェル) を解析しパーサ実装

**成果物**

- `main/uwb/uwb_uart.c`: UART 受信 + パース実装
- パース済みデータ構造:
  ```c
  typedef struct {
      uint8_t  tag_id;          // 0x01 〜 0x07
      uint16_t distance_mm;     // 0 〜 65535
      int16_t  azimuth_deg;     // -180 〜 +180
      int16_t  elevation_deg;   // -90 〜 +90
      uint32_t timestamp_ms;
      uint8_t  rssi;            // Optional
  } uwb_measurement_t;
  ```
- `idf.py monitor` で `Tag 0x01: 1234mm, az=+30°, el=-5°` のような出力が出る

**詰まりポイントと対処**

- ボーレート不一致 → 順番に試す (9600/115200/460800/921600)
- 配線が逆 → 2BP/ESP32 とも TX/RX 入れ替えを試す
- データが何も来ない → 2BP のリセットボタン長押し、USB給電状態確認、2DK の電源確認
- ASCII で読めない → binary プロトコルかもしれない、まず hex で観察
- Murata のドキュメントが NDA → サンプルコード (mw-tools or NXP MCUXpresso のサンプル) のシリアル出力部を読む

---

### Phase 2: レーダー UI 骨格 【5/1 (金)】

**目的**: LVGL で丸型ディスプレイに動くドラゴンレーダー風 UI を作る。実データはまだ繋がない。

**タスク**

1. ESP-IDF v5.3+ プロジェクト作成、LVGL v9.x コンポーネント追加
2. Waveshare 公式 GitHub から ST77916 driver と GT911 タッチ driver を取得・組込
3. 360×360 の `lv_disp` 初期化、円形マスク設定 (画面四隅は黒)
4. 背景レイヤ:
   - 真っ黒背景 (#000000)
   - 同心円 3 本 (半径 60, 120, 180 px、蛍光緑 #00FF66, opacity 60%)
   - 十字ガイド (90度刻み、蛍光緑 opacity 30%)
5. スイープ線:
   - 中央から外周への扇形 (`lv_arc`)、1.5 秒/周回で回転
   - 蛍光緑、opacity が中央→外周で減衰
6. 光点レイヤ:
   - 最大7個分の `lv_obj_t *dot[7]` を予め確保
   - 各タグID に色を割当 (赤/橙/黄/緑/青/紺/紫 = ドラゴンボール7色)
   - ダミーデータで円周を回転させて動作確認
7. 集めた数表示:
   - 画面右上に星アイコン×取得数 (PNG または SVG)

**成果物**

- `main/ui/radar_view.c`
- ダミーデータで7個の光点が好き勝手に動くデモ
- 起動から3秒以内に表示開始

**LVGL 実装メモ**

- 光点は毎フレーム再生成せず、`lv_obj_set_pos()` で座標更新のみ
- 距離→半径マッピングは log スケール推奨: `r = 180 * log(1 + d/1000) / log(11)` (10m で外周)
- スイープ線は `lv_arc` で範囲を毎フレーム更新
- フレームレート: 30 fps 目標、`LV_DISP_DEF_REFR_PERIOD = 33`
- カラー深度: `LV_COLOR_DEPTH=16`、PSRAM 有効化必須

---

### Phase 3: 実データ繋ぎ込み + ゲームロジック 【5/2 (土)】

**目的**: UART パーサと UI を繋ぎ、「タグを集める」ゲームロジックを実装する。

**タスク**

1. UART タスクと UI タスクを FreeRTOS queue で疎結合に
   - `xQueueSend(uwb_queue, &measurement, 0)` (最新値を捨てない、queue length=10)
   - UI タスクは 33ms ごとに最新値を読んで描画
2. 移動平均フィルタ実装:
   - `main/uwb/uwb_filter.c`: window=5 の単純移動平均
   - 距離が 0mm or 50000mm 以上は異常値として除外
3. ゲーム状態機械:
   ```
   IDLE → SEARCHING → COLLECTING(found_count++) → SUMMONING → WISH → END → IDLE
   ```
   - `IDLE`: タイトル画面、タッチで開始
   - `SEARCHING`: レーダー描画中
   - `COLLECTING`: タグから 30cm 以内に 2 秒滞在で「取得」、効果音 + 星追加
   - `SUMMONING`: 7個取得で発火、神龍召喚演出
   - `WISH`: ウーロン音声 + パンティ演出
   - `END`: スター散らばりアニメ → IDLE へ
4. 取得済みタグはレーダーから消す (光点を非表示)
5. 状態遷移ログをシリアルに出す (デバッグ用)

**成果物**

- `main/game/game_state.c`
- `main/uwb/uwb_filter.c`
- 1個ずつ近づくとちゃんと「集まった」判定が出る
- 取得時に右上の星が増える

**注意点**

- AoA は屋内マルチパスでフラフラする → フィルタ必須
- 「30cm 以内」判定は閾値を可変にしておく (デモ動画撮影時に調整)
- タグID の自動マッピングが必要 (2DK 7個分、初回検出順に色割当 or 固定UUID)

---

### Phase 4: 神龍演出 + ウーロン音声 【5/2 (土) 夜】

**目的**: 7個揃った瞬間の演出を仕上げる。コンテストで一番映える部分。

**タスク**

1. VOICEVOX (ホスト PC で事前生成) で WAV 出力:
   - **shenron_appear.wav**: 「神よ…願いを言え…何でも一つだけ叶えてやろう…」(低音・荘厳・ずんだもん？四国めたん？要選定)
   - **oolong.wav**: 「ギャルのパンティおくれーっ！」(高音・早口、これは絶対外せない)
   - **shenron_grant.wav**: 「よかろう…」
   - 全て 16kHz / 16bit / mono / WAV
2. SD カードから WAV を読込み、I2S で再生:
   - `esp_audio` または `esp-adf` フレームワーク採用
   - Waveshare の I2S 出力先 (内蔵ブザー or 外付け MAX98357A) を確認
3. 神龍召喚アニメ (`main/ui/summon_view.c`):
   - 画面が緑→金にフェード (3秒)
   - 雷エフェクト: ランダム位置に白線を `lv_canvas_draw_line()` で 0.2秒だけ描画 × 5回
   - 龍シルエット PNG をフェードイン (画面中央、10秒滞在)
4. パンティ演出:
   - 画面下から PNG が左右に揺れながら降ってくる (`lv_anim` で y, x 同時)
   - 3〜5枚を時差で出現、5秒で完了
5. 終了演出:
   - 集めた星7個が四方に飛び散る
   - 黒画面フェード → タイトルへ

**成果物**

- `main/audio/audio_player.c`
- `assets/audio/*.wav` (4ファイル)
- `assets/images/*.png` (PNG → LVGL `.c` ヘッダ変換 or SD から都度読込)

**実装メモ**

- 音声合成はリアルタイムでやらない、必ず事前生成
- I2S DMA バッファサイズ 1024、queue 8 程度
- PNG は LVGL Image Converter で C 配列化したほうが速い
- パンティは透過 PNG、適度にぼかすと品の悪さが軽減（適度に残すのが良い）

---

### Phase 5: デモ動画撮影 【5/4 (月)】

**目的**: ProtoPedia 用 2分以内のデモ動画を撮る。GW最終日、子守も復帰する直前。

**シナリオ案** (詳細は `docs/demo-scenario.md`)

| 時間 | 内容 |
|------|------|
| 0:00-0:10 | タイトル: 「Dragon Radar UWB - DigiKey Make ONE Challenge 2026」 |
| 0:10-0:25 | ハード紹介: Murata 2BP/2DK、Waveshare 丸型 LCD のアップ |
| 0:25-0:40 | 「自分イチ」テーマ説明: 初 UWB? 初 LVGL? 字幕で軽く |
| 0:40-1:30 | 公園で子供がタグ7個を探すデモ。手元アップ + 子供の表情 + 引き |
| 1:30-1:45 | 7個目を見つけた瞬間、神龍召喚 |
| 1:45-1:55 | ウーロン降臨「パンティおくれーっ」 |
| 1:55-2:00 | クレジット |

**撮影 Tips**

- 朝〜午前中の自然光で撮る (画面が見やすい)
- 三脚 + スマホで定点 + GoPro/手撮りで動きの2系統
- 子供の声は別マイクで拾えると最高
- パンティ演出は必ず子供のリアクションを撮る (これが一番映える)
- 編集は ffmpeg or DaVinci Resolve、字幕は CapCut でも可

---

## ProtoPedia 応募チェックリスト

応募ページに登録する項目:

- [ ] 作品タイトル: 「Dragon Radar UWB ～ドラゴンボールを探せ～」(仮)
- [ ] 一般公開設定で登録
- [ ] デモ動画 URL (YouTube 限定公開 OK、2分以内)
- [ ] myLists 部品表 URL
  - DigiKey で買った 2BP + 2DK + Waveshare の購入履歴をリスト化
  - DigiKey の「おすすめ製品」を含む（加点対象）
- [ ] 「自分イチ」テーマの説明
  - 候補: 「初めての UWB」「初めての LVGL」「初めての音声合成統合」「自分史上1番難しい配線」など
- [ ] 使用パーツの全 SKU 記載
- [ ] 開発過程の写真 (配線、デバッグ画面、子供がデモしている写真)
- [ ] ソースコード公開: GitHub リンク (MIT or Apache-2.0)

---

## 依存関係

**ESP32-S3 側 (ESP-IDF)**

- ESP-IDF v5.3+
- LVGL v9.x (`lvgl/lvgl` ESP-IDF コンポーネント版)
- ST77916 LCD driver (Waveshare 公式 or `esp_lcd` ベースで自作)
- GT911 タッチ driver (`esp_lcd_touch_gt911`)
- esp_audio or esp-adf (WAV 再生)
- FATFS (SD カードアクセス)

**ホスト側ツール**

- VOICEVOX (https://voicevox.hiroshiba.jp/) - 音声生成
- ffmpeg - 動画編集 + WAV フォーマット変換
- LVGL Image Converter (https://lvgl.io/tools/imageconverter) - PNG→C配列
- DaVinci Resolve or CapCut - 動画編集

---

## トラブルシューティングメモ

| 症状 | 対処 |
|------|------|
| 2BP がうんともすんとも | リセットボタン長押し、USB 給電確認、別 USB ポートで試す |
| LCD が真っ黒 | backlight GPIO HIGH 確認、SPI clock を 10MHz まで下げる、PSRAM 有効化確認 |
| LVGL がカクつく | `LV_COLOR_DEPTH=16`、PSRAM 有効化、frame buffer を internal RAM に |
| UART 文字化け | ボーレート再確認、GND 共通確認、配線長を 10cm 以内に |
| AoA がフラフラ | 移動平均 window を 10 まで増やす、屋外で撮影 |
| WAV が再生されない | I2S ピン番号確認、サンプルレート 16kHz / 16bit / mono か確認 |
| 7個タグ識別できない | 2DK 個体ごとの UUID を事前にメモして config に書く |

---

## 5/5 以降 (応募までの仕上げ + ストレッチゴール)

- 残り 5 個の 2DK EVK 追加発注 (DigiKey)
- 3D プリント筐体 (DMM.make でナイロンか透明アクリル)
- 7個目発見時に LED ストリップで派手な演出 (WS2812B を Grove で外付け)
- バッテリ駆動化 (LiPo + 充電回路)
- README.md を英語化 (DigiKey 本社にも刺さる可能性)
- 子供にレビューさせて UI 調整

---

## Claude Code への指示

このドキュメントを `dragon-radar/README.md` として置き、Claude Code に以下の手順で進めさせる:

1. Phase 1 から順番に着手。各 Phase 完了後に成果物を確認してから次へ進む。
2. Phase 1 で詰まったら、まず raw bytes ダンプを取得してそれを共有する (パース実装は後回しでOK)。
3. 各タスクは git commit を細かく切る (Phase ごとに最低3コミット)。
4. 完了したタスクはチェックボックスにチェックを入れる。
5. 詰まったら作業をブロックして人間に質問する (推測で進めない)。
6. ESP-IDF のビルドエラーは抱え込まずに即報告。
7. ハードウェアに依存する確認 (LCD 表示、UART 受信、音声出力) は人間が実機で確認、結果をフィードバック。

開発開始日: 2026-04-29
GW 集中期間終了: 2026-05-04
コンテスト応募締切: 2026-06-22
Maker Faire Tokyo 2026 (招待されれば): 2026-09-05〜06
