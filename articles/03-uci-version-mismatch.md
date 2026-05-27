# UWB ranging が動かない真犯人は「UCI 世代差」だった話

## 要約

異なる NXP UWB チップ (SR150 / SR040) を載せた 2 つの評価ボードを ranging させようとしたら、

- 両者の `RANGE_DATA_NTF` (測距結果通知) に乗っている **session ID が違って見える**
- 一方は `0x00000001`、もう一方は `0x11223344`

「session ID が違うから pair しないんだ」と最初は誤診したが、SDK のソースを当たると **両方ともデフォルトで `0x11223344` を設定している**ことが判明。

実際に違うのは UCI (UWB Command Interface) のプロトコル世代で、UCI v1.x は `Session ID` を payload にそのまま乗せるが、UCI v2.0 以降は `Session Handle` というラッパに置き換わる。**通知フィールドが見た目同じバイト位置でも、解釈が違うので一致しない**。

両方を **UCI v1.31 世代の SDK に揃えた瞬間に ranging が成立**した。誤診から本質に辿り着くまでの記録。

## 環境

- Initiator: NXP SR150 ベースの評価ボード (Murata Type 2BP EVK Rev 4.1)
- Responder: NXP SR040 ベースの評価ボード (Murata Type 2DK EVK)
- 両者にメーカープリビルド済の standalone ranging デモを焼いて、UART 経由で生の UCI トレースを観察

## 最初の観測

電源を入れて並べると、2 つの EVK はとりあえずお互いの存在は認識する (TX/RX が走る) のだが、**ranging が完了しない**。Murata 提供の Python テストスクリプトも distance を 1 度も print しないまま終わる。

両方の UART を `cat /dev/ttyUSB*` で生キャプチャして UCI フレームを抜き出すと、

- 2BP (SR150 / 新しい SDK) 側: NTF 内の session 識別フィールドが `01 00 00 00` (リトルエンディアン解釈で `0x00000001`)
- 2DK (SR040 / 古い SDK) 側: 同じ位置が `44 33 22 11` (リトルエンディアン解釈で `0x11223344`)

ぱっと見「2 つの session ID が違う、だから ranging が成立しない」と読みたくなる。これが間違いだった。

## SDK のソースを当たる

それぞれの SDK のサンプルコードを `grep -nrR SESSION_ID` する。

```
demos/SR1XX/demo_ranging_controller/demo_ranging_controller.c:
  #define RANGING_APP_SESSION_ID 0x11223344

demos/SR040/demo_tracker_sr040/app_Ranging_Cfg.h:
  #define RANGING_APP_SESSION_ID 0x11223344
```

両方とも **`0x11223344` を設定している**。つまりアプリケーション層で投げているセッション ID は一致しているはず。

ということは、UART で見ている「session 識別フィールド」がそもそも別物を指している可能性が高い。

## UCI 仕様書を読み直す

NXP の SR150 UCI Specification を当たると、v1.x と v2.0 で `RANGE_DATA_NTF` の最初の数バイトの定義が変わっている:

| バイト位置 | UCI v1.x | UCI v2.0 |
|---|---|---|
| 0-3 | Sequence Number | Sequence Number |
| 4-7 | **Session ID** (アプリが設定した 32-bit 値そのまま) | **Session Handle** (UCI スタックが割り当てた識別子) |
| 8-11 | Rcr Block Index | Rcr Block Index |
| ... | ... | ... |

v2.0 で導入された **Session Handle** は SDK 内部で `SessionInit` 時に動的に発行される (例: `0x00000001` から順に割り当てる) 識別子で、アプリが設定した Session ID とは別物。アプリ層 API では Session ID を渡すが、UCI バイナリ層では Handle に置き換わる。

つまり、

- SR150 + 新しい SDK: UCI v2.0 → NTF の 4-7 バイト目には **Session Handle (`0x00000001`)** が乗る
- SR040 + 古い SDK: UCI v1.x → NTF の 4-7 バイト目には **Session ID (`0x11223344`)** がそのまま乗る

両者は同じバイト位置に別の意味の値を入れていて、当然 ranging session の照合に失敗する。**「session ID が違う」のではなく「session 識別の規約が違う」が本質**だった。

## 揃え方

選択肢は 2 つ:

1. SR040 側の SDK を v2.0 世代に上げる
2. SR150 側の SDK を v1.x 世代に下げる

SR040 はメーカー (Murata) が校正済の standalone バイナリを v1.x 世代でしか出してくれていなかったので、現実解は (2)。**SR150 側を NXP UWBIOT SDK v04.04.03 (UCI v1.31) の bare バイナリに差し替え**、両者の UCI 世代を v1.31 に統一した。

結果: 焼き直して電源入れた瞬間、UART に `TWR[0].distance : 26` のような distance 値が湯水のように流れ始めた。20 秒で 200+ サンプル、distance は両端で平均 0.5cm 以内に一致 (DS-TWR が正しく完走している証拠)。

## 教訓

- **「session ID が違う」のように『見える』状態を素直に取らない**。NTF のバイト位置と UCI 仕様書の世代を必ず照合する
- **異種チップを組ませる時は UCI 仕様の世代揃えが必須**。SR150 と SR040 は同じ NXP 製でも世代が異なる SDK で出てくる
- **デバッグの第 1 歩は SDK ソースの grep**。「定数として何を設定しているか」を直接確認すれば、UART で観測した値との差分が見える

UCI が業界標準 (FiRa Consortium 仕様) になりつつあるとはいえ、世代差を吸収するレイヤはまだ薄い。**UWB を触る時は両端の SDK バージョンを意識して揃えること**が、ranging を素直に動かす近道。
