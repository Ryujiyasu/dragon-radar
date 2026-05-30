#!/usr/bin/env bash
# 印刷用 STL を一括書き出し。
# OpenSCAD は PATH の `openscad`、無ければ環境変数 OPENSCAD を使う。
#   例: OPENSCAD=/tmp/squashfs-root/AppRun ./export-stl.sh
set -euo pipefail
cd "$(dirname "$0")"

OSCAD="${OPENSCAD:-openscad}"
SRC="dragon-radar-enclosure.scad"
OUT="stl"
FN="${FN:-180}"   # 出力品質 ($fn)。確認は120、最終は180+

mkdir -p "$OUT"

# 印刷する部品 (assembly/board/coupon は除外)
PARTS=(bezel body back coupon_body coupon_bezel)

for p in "${PARTS[@]}"; do
  echo "==> $p"
  "$OSCAD" -o "$OUT/dragon-radar-$p.stl" \
           -D "part=\"$p\"" -D "\$fn=$FN" "$SRC"
done

echo "完了: $OUT/ に $(ls "$OUT"/*.stl | wc -l) ファイル"
