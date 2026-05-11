# 2DK QN9090 カスタムファームウェア (Path C)

タグ側 (2DK EVK) の QN9090 で動かす Responder 専用 FW。電源 ON で待機し、2BP からの TWR 要求に自動応答する。

## 必要な前提

- 2BP と同じ (NXP UWBIOT SDK / Murata パッチ / MCUXpresso / MCU-Link Pro / ARM GCC)
- ただし SR040 用の demo_ranging_responder サンプルをベースにする
- Murata 出荷時 FW は Initiator 固定なので、**この Responder FW は必須**

## ビルド手順 (Phase 3 で確定)

```bash
cd firmware/2bp-config/sdk/UWBIOT_SR150_*_SEGGER/uwbiot-top
cd boards/Host/Rhodes4
make demo_ranging_responder TARGET=QN9090 SR040=1
```

## 設定項目

各 2DK ごとに**異なる UWB MAC アドレス**を設定する (ranging session で識別するため):

```c
// config.h
#define UWB_DEVICE_MAC      0xABCD0001  // 個体毎に変える: 0001, 0002, 0003 ...
#define RANGING_TIMEOUT_MS  100         // controller からの poll 待機 timeout
#define LOW_POWER_MODE      1           // CR2032 駆動なので消費電流抑制
```

## ファイル配置 (Phase 3 で実装予定)

```
2dk-fw/
├── README.md       (このファイル)
├── src/
│   ├── main.c
│   ├── responder.c/h    # SR040 を Responder として常駐
│   └── config.h         # UWB MAC、低電力設定
└── prj/                 # MCUXpresso プロジェクト
```

## 個体管理

7 個の 2DK タグに以下の情報を物理的にラベリングしておく:

| タグ ID | UWB MAC | ドラゴンボール色 | シリアル番号 (基板) |
|---|---|---|---|
| 1 | 0xABCD0001 | 赤 | (Phase 3 で記入) |
| 2 | 0xABCD0002 | 橙 | |
| 3 | 0xABCD0003 | 黄 | (9月用追加分) |
| 4 | 0xABCD0004 | 緑 | |
| 5 | 0xABCD0005 | 水色 | |
| 6 | 0xABCD0006 | 青 | |
| 7 | 0xABCD0007 | 紫 | |
