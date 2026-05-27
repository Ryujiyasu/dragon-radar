# MCUXpresso IDE の headless build で組込ファームを 10 秒で回す

## 要約

NXP が出している MCUXpresso IDE は Eclipse CDT ベースなので、見た目は GUI でも中身は **コマンドラインから単独でビルドを叩ける**。

`mcuxpressoide -application org.eclipse.cdt.managedbuilder.core.headlessbuild` という呪文を覚えてしまえば、IDE を立ち上げる時間も、Eclipse のウィンドウマネージャ周りで詰まる時間もゼロにできて、**1 回 9 秒** でビルドが回るようになる。

「IDE で開いて、メニューから Build をクリックして、終わるのを待って」というループを 10 倍速くしたい人向けの記事。

## なぜ GUI で回すと遅いか

NXP UWBIOT SDK のようなボリュームある SDK を MCUXpresso GUI で開くと、

1. ワークスペースを開くだけで 30 秒以上
2. インデックス再構築で CPU 100% が 2〜3 分
3. Build のたびに「Project Indexer Update」が走り直して数十秒

ソースを 1 行直して焼き直したいだけなのに毎回これに付き合うのは消耗する。

Eclipse CDT は元々 IBM (JDT) から派生した CDT 部分でヘッドレスビルダを持っていて、`-application org.eclipse.cdt.managedbuilder.core.headlessbuild` を渡せば **IDE の UI を起動せずに同じツールチェーンで .project / .cproject をビルドできる**。

## 環境

- Ubuntu 24.04
- MCUXpresso IDE for Linux (`/usr/local/mcuxpressoide-25.6.136/ide/`)
- 内蔵 JRE (OpenJDK 17) と内蔵 arm-none-eabi-gcc を使う

## ヘッドレスビルド呪文

```bash
xvfb-run --auto-servernum --server-args="-screen 0 1024x768x24" \
  /usr/local/mcuxpressoide-25.6.136/ide/mcuxpressoide \
  -nosplash \
  -application org.eclipse.cdt.managedbuilder.core.headlessbuild \
  -data /tmp/mcux-ws \
  -import /path/to/project/MyProject \
  -cleanBuild "MyProject/Debug"
```

各オプションの意味:

| オプション | 意味 |
|---|---|
| `xvfb-run` | Linux サーバや CI で X が無くても Eclipse の GTK 依存を満たす仮想ディスプレイを立てる |
| `-nosplash` | スプラッシュ画面を出さない (ヘッドレスでは不要) |
| `-application <id>` | Eclipse プラットフォームの起動アプリケーションを指定。CDT のヘッドレスビルダはこの ID |
| `-data <dir>` | Eclipse ワークスペース。事前に存在しなくても自動作成される |
| `-import <project>` | `.project` のあるディレクトリ。プロジェクト名はその中身から読み取られる |
| `-cleanBuild "Project/Config"` | クリーンビルドする対象を「プロジェクト名/ビルドコンフィグ名」で指定 |

ポイントは **`Project/Config` がプロジェクト内の `.cproject` に書かれている名前と完全一致しないとダメ** という所。GUI で右クリック → Properties → C/C++ Build → 設定タブのリスト名と一致する。

`.cproject` ファイル内で:

```xml
<configuration name="Debug" ...>
<configuration name="Release" ...>
```

これを grep するのが手っ取り早い。

## 結果

筆者のケース (NXP UWBIOT SDK v04.04.03 + Murata Type 2BP 向け demo_ranging_controller) では:

```
[1/12] Performing build step for 'bootloader'
...
[10/12] Linking CXX executable RhodesV4_SE.axf
Memory region         Used Size  Region Size  %age Used
   PROGRAM_FLASH:      353180 B     629471 B     56.11%
            SRAM:       32576 B        87 KB     36.57%
           SRAM1:         45 KB        64 KB     70.31%
[11/12] Generating binary image
...
Build Finished. 0 errors, 1 warnings. (took 9s.620ms)
```

**9.6 秒**で .axf + .bin まで生成される。GUI で開きっぱなしより速い (差分ビルドなら GUI でも数秒だが、起動時間を含めると勝てない)。

## おまけ: シェル関数化

毎回タイプするのは流石に辛いので zshrc / bashrc に入れておく:

```bash
mcux_build() {
  local proj="$1"
  local cfg="${2:-Debug}"
  xvfb-run --auto-servernum --server-args="-screen 0 1024x768x24" \
    /usr/local/mcuxpressoide-25.6.136/ide/mcuxpressoide \
    -nosplash \
    -application org.eclipse.cdt.managedbuilder.core.headlessbuild \
    -data /tmp/mcux-ws \
    -import "$proj" \
    -cleanBuild "$(basename "$proj")/$cfg"
}

# 使う側
mcux_build /path/to/MyProject Debug
```

これで `mcux_build .` のような短い呼び出しで CI でも開発でも使える。

## ハマりどころメモ

| 症状 | 原因 / 対処 |
|---|---|
| `No projects matched` | `-cleanBuild` の引数が `.cproject` のコンフィグ名と一致してない |
| `DISPLAY 環境変数が設定されてない` | xvfb-run を使う、もしくは X 転送 (`ssh -X`) |
| `Workspace not initialized` | `-data` で指定したパスのパーミッションが書込み不可。`/tmp/` 以下が無難 |
| ビルドが空のサイレント終了 | `-import` のパスが間違っているとログも吐かず終わることがある。絶対パス推奨 |

## 何が良くなったか

- **編集 → ビルド → 焼き** のループが体感 1 桁速くなる
- **CI で組込ファームをビルド** できる。Pull Request ごとにファームのバイナリ差分を取れる
- IDE GUI のメモリ食いが消えるので、開発端末のリソースを LLM チャットや別 IDE に回せる

NXP に限らず Eclipse CDT ベースの組込 IDE (STM32CubeIDE 等) は同じ仕組みで動く。**「IDE を立ち上げてビルドする」習慣をやめると、組込開発の生産性は素直に上がる**。
