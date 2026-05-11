# 2BP QN9090 カスタムファームウェア (Path C)

ESP32 ↔ 2BP の自前 ASCII プロトコル ([../../docs/2bp-protocol.md](../../docs/2bp-protocol.md)) を喋り、内部で SR150 を UCI で叩いて round-robin Unicast TWR を実行する QN9090 用ファーム。

## 必要な前提

- NXP UWBIOT SDK SR150 v04.06.00 (要 NXP MyAccount)
- Murata 提供パッチ (`2bp_prebuilt_*.patch`、my.murata.com から取得)
- MCUXpresso IDE (Linux 版、無料)
- MCU-Link Pro (SWD 書込用)
- ARM GCC toolchain (`arm-none-eabi-gcc`)

## SDK 配置 (gitignore 内)

NXP からダウンロードした SDK は **NDA 配下** のため、コミット禁止。下記に展開する:

```
firmware/2bp-config/
├── docs/                             # Murata PDF (取得済)
└── sdk/                              # ★ ここに SDK を展開
    ├── UWBIOT_SR150_*_SEGGER/
    └── 2bp_prebuilt_*.patch
```

`firmware/2bp-config/.gitignore` で全配下が除外されているので安全。

## ビルド手順 (Phase 1 で確定)

```bash
# 1. SDK ディレクトリへ
cd firmware/2bp-config/sdk/UWBIOT_SR150_*_SEGGER/uwbiot-top

# 2. Murata パッチ適用 (Quick Start PDF 参照)
patch -p0 < ../2bp_prebuilt_*.patch

# 3. デモ ranging controller を 2BP の QN9090 用に make
cd boards/Host/Rhodes4
make demo_ranging_controller TARGET=QN9090

# 4. JLinkExe で書込み (MCU-Link Pro が J-Link 互換ファーム)
JLinkExe -device QN9030 -if SWD -speed 4000 -CommandFile flash.jlink
```

## 開発ステップ

| Phase | やること |
|---|---|
| 1 | SDK + パッチ適用 → Murata 出荷 FW 相当をビルド・書込・動作確認 |
| 2 | UART パーサ (ASCII) を組み込み、`VER`/`RANGE` (1タグ) 実装 |
| 3 | `SET_TAGS` / round-robin (2 タグ対応) |
| 7 | round-robin の 7 タグ対応、サイクル時間最適化 |

## ファイル配置 (Phase 2 で実装予定)

```
2bp-fw/
├── README.md       (このファイル)
├── src/
│   ├── main.c          # FreeRTOS エントリ
│   ├── uart_cmd.c/h    # ASCII コマンドパーサ
│   ├── ranging.c/h     # round-robin スケジューラ
│   └── config.h        # SR150 設定、タグ MAC 一覧
└── prj/                # MCUXpresso プロジェクト一式
```

## デバッグ Tips

- 元の Murata プリビルド FW に戻すには Phase 0 で取得した `firmware-backups/2bp_qn9090.bin` を JLink で書き戻す
- UART 出力は MCU-Link Pro の Bridge UART で覗ける (Virtual COM)
- SR150 への UCI コマンド送信前に必ず `phNxpUciHal_init()` を呼ぶ (校正データロード)
