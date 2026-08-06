#!/usr/bin/env bash

set -euo pipefail

URL='https://lfs.aminer.cn/lab-datasets/citation/dblp.v10.zip'
EXPECTED_SIZE=1848832450

REPO_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
ZIP_PATH="$REPO_ROOT/dataset/dblp/dblp.v10.zip"
DEST_DIR="$REPO_ROOT/dataset/dblp/dblp-ref-v10"
SENTINEL="$DEST_DIR/dblp-ref-0.json"

filesize() {
    stat -c %s "$1" 2>/dev/null || stat -f %z "$1"
}

mkdir -p "$DEST_DIR" "$(dirname "$ZIP_PATH")"

if [[ -f "$ZIP_PATH" ]] && [[ "$(filesize "$ZIP_PATH")" == "$EXPECTED_SIZE" ]]; then
    echo "[v10] zip already complete: $ZIP_PATH"
else
    echo "[v10] downloading $URL"
    echo "      -> $ZIP_PATH (~$((EXPECTED_SIZE / 1024 / 1024)) MB)"
    curl -L --fail --retry 3 --retry-delay 5 -C - -o "$ZIP_PATH" "$URL"
    got=$(filesize "$ZIP_PATH")
    if [[ "$got" != "$EXPECTED_SIZE" ]]; then
        echo "[v10] ERROR: size mismatch ($got != $EXPECTED_SIZE). Re-run to resume." >&2
        exit 1
    fi
    echo "[v10] download verified: $got bytes"
fi

if [[ -f "$SENTINEL" ]]; then
    echo "[v10] already unpacked: $DEST_DIR/"
else
    echo "[v10] unpacking into $DEST_DIR/"
    unzip -o -j "$ZIP_PATH" -d "$DEST_DIR"
    ls -lh "$DEST_DIR"
fi

echo "[v10] done. Next: python scripts/20260208-scripts/prep_dblp_v10.py"
