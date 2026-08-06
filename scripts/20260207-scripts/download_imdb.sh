#!/bin/bash

set -u

date
start_time=$(date +%s)

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "${SCRIPT_DIR}/../.." && pwd)
RAW_DIR=${REPO_ROOT}/dataset/imdb/raw
BASE_URL="https://datasets.imdbws.com"

files=(
	"title.principals.tsv.gz"
	"title.basics.tsv.gz"
	"name.basics.tsv.gz"
)

VERIFY=0
for arg in "$@"; do
	case "$arg" in
		--principals-only) files=( "title.principals.tsv.gz" ) ;;
		--verify)          VERIFY=1 ;;
		*) echo "unknown option: $arg" >&2; exit 2 ;;
	esac
done

if command -v wget >/dev/null 2>&1; then
	FETCH="wget -c"
elif command -v curl >/dev/null 2>&1; then
	FETCH="curl -fL -C - -O"
else
	echo "need wget or curl" >&2
	exit 1
fi

mkdir -p "$RAW_DIR"
avail_kb=$(df -Pk "$RAW_DIR" | awk 'NR==2 {print $4}')
if [ "$avail_kb" -lt 2000000 ]; then
	echo "WARNING: only $((avail_kb / 1024)) MiB free on $(df -Ph "$RAW_DIR" | awk 'NR==2 {print $6}')," >&2
	echo "         the three files need ~1.2 GB and the matrices built from them need more." >&2
fi

cd "$RAW_DIR" || exit 1
for f in "${files[@]}"; do
	echo "--- $f"
	$FETCH "${BASE_URL}/${f}" || { echo "failed to fetch $f" >&2; exit 1; }
done

echo "fetched $(date -u +%Y-%m-%dT%H:%M:%SZ) from ${BASE_URL}" > FETCHED

echo
echo "--- gzip integrity ---"
for f in "${files[@]}"; do
	if gzip -t "$f" 2>/dev/null; then
		echo "$f: ok"
	else
		echo "$f: CORRUPT -- delete it and re-run" >&2
		exit 1
	fi
done

if [ "$VERIFY" -eq 1 ]; then
	echo
	echo "--- row counts (decompresses ~9 GB, takes a few minutes) ---"
	for f in "${files[@]}"; do
		n=$(gzip -dc "$f" | wc -l)
		echo "$f: $((n - 1)) rows"
	done
fi

echo
ls -lh "$RAW_DIR"

echo
echo "next:"
echo "  python3 ${SCRIPT_DIR#$REPO_ROOT/}/imdb2mtx.py --name imdb_movie --title-types movie"

end_time=$(date +%s)
echo "Duration: $((end_time - start_time)) seconds"
date
