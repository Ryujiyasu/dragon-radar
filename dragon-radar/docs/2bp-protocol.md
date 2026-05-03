# 2BP ↔ ESP32 ASCII プロトコル仕様 (Path C)

ESP32-S3 と 2BP の QN9090 (カスタム FW) が UART 経由でやり取りする自前プロトコルの定義。

## 物理層

- **接続**: 2BP の USB-C (FTDI FT230XQ 経由 USB-CDC) → ESP32-S3 の USB ホスト or 直接 UART
- **想定ボーレート**: 115200 8N1 (Phase 1 で確定)
- **フロー制御**: なし

### 配線パターン (どちらか選択)

#### A) USB-CDC 経由 (推奨、ハード改造不要)
```
2BP USB-C ── USB ケーブル ── ESP32-S3 USB OTG (Host モード)
                              └─ esp_usb_host_cdc_acm で FTDI を喰う
```

#### B) 直 UART (USB が不安定なら)
```
2BP のヘッダ (UART_TX/RX) ── ESP32 GPIO (UART2)
※ ピン位置は QN9090 のサンプル FW のシリアル出力ピンを参照、Phase 1 で確定
```

## アプリケーション層 (ASCII コマンド)

行ベース、`\r\n` 区切り。コマンドは ESP32 → 2BP、レスポンスは 2BP → ESP32。

### コマンド一覧

| コマンド | 意味 | レスポンス例 |
|---|---|---|
| `RANGE\r\n` | 1 サイクル分の round-robin ranging を即時実行 | `OK TAG1:1234,30,-5 TAG2:2100,-15,40\r\n` |
| `START <interval_ms>\r\n` | 指定間隔で連続 ranging 開始 (例: `START 33`) | `OK\r\n` その後定期的に `RANGE_RESULT TAG1:...\r\n` |
| `STOP\r\n` | 連続 ranging 停止 | `OK\r\n` |
| `SET_TAGS <count> <mac1> <mac2> ...\r\n` | 追跡対象タグの UWB MAC を登録 | `OK\r\n` |
| `STATUS\r\n` | 現在状態を返す | `READY` / `RANGING` / `ERROR <code>` |
| `RESET\r\n` | QN9090 ソフトリセット | `OK\r\n` (リセット後再 init) |
| `VER\r\n` | カスタム FW のバージョン取得 | `VERSION 1.0.0 SDK 4.6.0\r\n` |

### レスポンス形式

#### 成功
```
OK [<key1>:<val1> <key2>:<val2> ...]\r\n
```

#### Ranging 結果 (TAG_n の値は CSV)
```
TAG<id>:<distance_mm>,<azimuth_deg>,<elevation_deg>[,<rssi>][,<flags>]
```
- `distance_mm`: 0-65535
- `azimuth_deg`: -180 〜 +180 (整数 deg、または -1800〜1800 で 0.1deg 精度)
- `elevation_deg`: -90 〜 +90
- `rssi`: 任意 (Phase 2 で必要なら追加)
- `flags`: 任意 (e.g. `LOS=1`, `FOM=100`)

#### エラー
```
ERROR <code> <message>\r\n
```
| code | 意味 |
|---|---|
| `NO_TAG` | 指定タグから応答無し |
| `BUSY` | 別操作中 |
| `BAD_CMD` | コマンドパース失敗 |
| `RF_FAIL` | UCI レイヤエラー |

### 通信例

```
# ESP32 起動時
ESP32: VER\r\n
2BP : VERSION 1.0.0 SDK 4.6.0\r\n

# タグ登録
ESP32: SET_TAGS 2 ABCD1234 ABCD5678\r\n
2BP : OK\r\n

# 1 サイクル要求
ESP32: RANGE\r\n
2BP : OK TAG1:1234,30,-5,FOM=100 TAG2:2100,-15,40,FOM=85\r\n

# 連続モード開始
ESP32: START 33\r\n
2BP : OK\r\n
2BP : RANGE_RESULT TAG1:1240,32,-6,FOM=100 TAG2:2098,-14,41,FOM=88\r\n
2BP : RANGE_RESULT TAG1:1235,31,-5,FOM=100 TAG2:NO_RESPONSE\r\n
ESP32: STOP\r\n
2BP : OK\r\n
```

## QN9090 側の実装方針

- FreeRTOS タスク 2 個:
  - **UART タスク**: stdin から 1 行読んでコマンドキューへ enqueue
  - **Ranging タスク**: コマンドキューを読んで NXP UCI Host stack を駆動
- round-robin: 設定済みタグを順に Unicast TWR、結果をまとめて 1 行で返却
- 1 タグあたり TWR 約 10ms 想定 (要実測)、2 タグで 20ms、7 タグで 70ms 程度
- ESP32 側のフレームレート (LVGL 30Hz = 33ms) に合わせるなら 7 タグはギリギリ

## ESP32 側の実装方針

- `main/uwb/uwb_uart.c`: 上記レスポンスのパーサ
- 結果を `uwb_measurement_t` 構造体に詰めて FreeRTOS queue へ
- LVGL タスクは queue から最新値を読んでレーダー描画

## Phase ごとの実装ステップ

| Phase | 範囲 |
|---|---|
| Phase 1 | 出荷時 FW で raw UCI を眺めるだけ (このプロトコルは未使用) |
| Phase 2 | 2BP に最小カスタム FW、`VER` `STATUS` `RANGE` (1 タグ) を実装 |
| Phase 3 | `SET_TAGS` `START`/`STOP`、2 タグ round-robin |
| Phase 7 | 7 タグ拡張、サイクル時間チューニング |

## 参考にする Murata ドキュメント

`firmware/2bp-config/docs/` 配下 (NDA):
- Type 2BP Evaluation Board Quick Start Guide_EVK_Rev4.2.pdf
- Type 2BP UWB Module EVK with Raspberry Pi4 Linux_RevD.pdf (UART 接続例)
- SR150_UCI_Specification_v1.23_Murata.pdf (UCI フレーム形式)
- Type 2BP EVK Enabling AoA Setting_RevC.pdf (AoA 校正)
