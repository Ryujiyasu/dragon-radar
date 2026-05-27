# dk6prog (SPSDK) で QN9090 を USB ISP だけで焼く

## 要約

NXP QN9090 を載せた評価ボード (Murata Type 2BP EVK / 2DK EVK 等) のファームを書き換えるとき、SWD (J-Link / MCU-Link Pro) が標準ルートとされるが、ボードのデバッグピンが折れていたり配線できない事情があるとそこで詰まる。

そこで便利なのが **SPSDK** に含まれる `dk6prog` という Python ツールで、USB ケーブル 1 本だけで ISP (In-System Programming) モードに入って書き換えできる。

本記事では Linux 上で `pip install spsdk` だけで導入して、QN9090 のフラッシュ全領域を吸い出してバックアップ → カスタムファームを書き込む手順を 1 枚にまとめる。

## なぜこれが効くか

QN9090 (および兄弟の DK6 シリーズ) は ROM ブートローダに ISP モードを備えていて、特定ピン (PIO_5 / ISP_ENTRY) を引いた状態でリセットすると UART 経由でフラッシュ操作を受け付ける。Murata / NXP の DK6 EVK はオンボードに FTDI FT230X USB-UART ブリッジが載っていて、USB ケーブル 1 本でこの UART に到達できる。

SWD で書ける環境を持っていない人 (J-Link を持っていない、MCU-Link Pro が手元にない、開発端末が Windows 縛りで動かないなど) でも、**Python が動けば書ける**のが大きい。

## 手順

### 0. 前提

- 開発端末: Ubuntu 24.04 (Linux 一般、macOS でも同様)
- ターゲット: NXP QN9090 ベースの DK6 EVK (例: Murata Type 2BP / 2DK)
- USB ケーブル (データ通信対応 micro-USB、充電専用品はダメ)

### 1. SPSDK をインストール

```bash
pip install spsdk pyftdi
```

(venv 推奨。SPSDK は依存が多めなので素の Python に入れると他プロジェクトと衝突しやすい。)

### 2. USB 接続して認識を確認

EVK と PC を USB でつないだ状態で:

```bash
dk6prog listdev
```

```
List of available devices:
DEVICE ID: DM86TTWC, VID: 0x403, PID: 0x6015, ...
```

`DEVICE ID` がそのまま `-d` の引数になる。Murata 出荷品は全 EVK で同じシリアルが振られていることがあり、複数挿していると後続コマンドで衝突するので、**作業時は対象 1 個だけ挿す**。

### 3. ISP モードに入れて疎通確認

EVK の **ISP ボタン (PIO_5 / ISP_ENTRY) を押しながら RESET を離す**と ISP モードに入る (ボードによっては「ISP」シルク印刷がある専用ボタン、ない場合は基板上のテストパッドを GND に落とす)。

```bash
dk6prog -d DM86TTWC info
```

```
Chip ID: 0x88888888
ROM Version: 0x140000cc

  Memory   Memory ID   Base Address   Length    Sector Size
----------------------------------------------------------------
  FLASH    0           0x0            0x9de00   0x200
  PSECT    1           0x0            0x1e0     0x10
  ...
```

`info` が返ってくれば疎通 OK。

### 4. 焼く前に「出荷時 FW」をバックアップする

これが本記事で一番伝えたい部分。Murata の EVK は出荷時にメーカー校正済の FW が入っており、これを潰すと TX パワーや水晶発振の校正が失われる。

```bash
dk6prog -d DM86TTWC read 0 0x9DE00 -o factory_backup.bin
```

`0x9DE00` (約 632KB) が FLASH 全長。1 度だけ取って、git に上げる代わりに「火事から助かる」場所に保管しておくと心理的に強い。

### 5. カスタムファームを書き込む

```bash
dk6prog -d DM86TTWC erase 0 0x9DE00
dk6prog -d DM86TTWC write 0 my_custom_fw.bin
```

(erase は `write` の中で勝手にやってくれる版もあるが、明示的に erase してから write する方が事故が少ない。)

### 6. 復旧したいとき

```bash
dk6prog -d DM86TTWC erase 0 0x9DE00
dk6prog -d DM86TTWC write 0 factory_backup.bin
```

これで step 0 の状態に戻る。安心。

## ハマりどころメモ

| 症状 | 原因 / 回避 |
|---|---|
| `2 USB devices match URL` | 同じシリアルの EVK が複数挿さっている。物理的に片方を抜く |
| `no langid permission issue` | pyftdi backend は libusb で root 権限要求することがある → `sudo -E` で実行 (環境変数を保持しないと PATH が消えるので `-E` 必須) |
| `Failed to enter ISP mode` | ISP ボタン押しが遅い / 早い。USB を抜いてボタン押しっぱなしで挿し直す → リセットボタンも併用 |
| `Chip ID: 0x88888888` のまま | これは正常 (ISP モードで MAC アドレスが返らない仕様) |

## 何が良くなったか

- **SWD ハードが要らない** (J-Link / MCU-Link Pro は 1 台 1〜2 万円)
- **OS を選ばない** (Windows 限定の DK6Programmer.exe に縛られない)
- **CI に組み込める** (シェルから叩けるので、ビルド → 焼き → テスト を一筆書きできる)

「SWD ピンが曲がってた」「IDE のドライバが入らない」みたいな ハードウェアやアーキテクチャに本質的でない問題で停滞している人にとって、`dk6prog` 経由のフラッシュは数時間〜数日を取り戻すポテンシャルがある。

## 参考

- [NXP SPSDK GitHub](https://github.com/nxp-mcuxpresso/spsdk)
- DK6 ファミリ User Manual (NXP)
