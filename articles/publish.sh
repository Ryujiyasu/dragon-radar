#!/bin/bash
# Publish all articles in this directory to yasu-home.com WP as published posts in
# the "IoT・組込み" (slug: iot) category, owned by user "ryuji".
#
# Each *.md file is converted to Gutenberg HTML via convert.py, uploaded,
# and `wp post create`d through SSH with PHP 7.4.33.

set -euo pipefail

HOST=xs832503.xsrv.jp
WP_ROOT='~/yasu-home.com/public_html'
PHP=/opt/php-7.4.33/bin/php
WP=/usr/bin/wp
CATEGORY_SLUG=iot
AUTHOR_LOGIN=ryuji
REMOTE_TMP=/tmp/dragon-articles
ARTICLES_DIR="$(cd "$(dirname "$0")" && pwd)"

ssh "$HOST" "mkdir -p $REMOTE_TMP"

for md in "$ARTICLES_DIR"/[0-9][0-9]-*.md; do
    base="$(basename "$md" .md)"
    echo "=== Processing $base ==="

    # Convert markdown to title + body
    out="$(python3 "$ARTICLES_DIR/convert.py" "$md")"
    title="$(printf '%s\n' "$out" | sed -n '1s/^TITLE: //p')"
    # body = everything after the ---BODY--- separator
    body_file="/tmp/${base}.body.html"
    printf '%s\n' "$out" | awk '/^---BODY---$/{f=1; next} f' > "$body_file"

    echo "  title: $title"
    echo "  body: $(wc -c < "$body_file") bytes"

    # Upload body to the remote temp dir
    remote_body="$REMOTE_TMP/${base}.body.html"
    scp -q "$body_file" "$HOST:$remote_body"

    # Create the post (published) under the iot category
    # Use a heredoc that runs wp post create on the server with proper quoting
    ssh "$HOST" bash -s <<REMOTE
set -e
cd $WP_ROOT
$PHP $WP post create $remote_body \
    --post_type=post \
    --post_status=publish \
    --post_title='${title//\'/\'\\\'\'}' \
    --post_author=$AUTHOR_LOGIN \
    --post_category=$CATEGORY_SLUG \
    --porcelain 2>/dev/null
REMOTE
done

echo "=== Done. Posts on https://yasu-home.com/ ==="
