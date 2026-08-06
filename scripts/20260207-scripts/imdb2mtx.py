import argparse
import gzip
import json
import os
import sys
from array import array

try:
    import numpy as np
except ImportError:
    raise SystemExit("imdb2mtx.py needs numpy: pip install numpy")

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_SRC = os.path.join(REPO_ROOT, "dataset", "imdb", "raw")
TAMU_OUT = os.path.join(REPO_ROOT, "dataset", "tamu")
GROUPBY_OUT = os.path.join(REPO_ROOT, "dataset", "groupby-sorted")

WRITE_CHUNK = 1 << 20
PROGRESS_EVERY = 10_000_000


TRIPLE_DT = np.dtype([("user_id", "<i8"), ("item_id", "<i8"), ("rating", "<f8")])


FLOP_P90 = 2_099_952_367
FLOP_MAX = 15_574_082_986
FLOP_WARN = 20_000_000_000


def read_title_types(path, wanted):
    if not wanted:
        return None
    if not os.path.isfile(path):
        raise SystemExit(
            f"missing {path}\n--title-types needs title.basics.tsv.gz; run "
            f"download_imdb.sh without --principals-only")
    keep = array("q")
    n = 0
    with gzip.open(path, "rb") as fh:
        fh.readline()
        for line in fh:
            n += 1
            f = line.split(b"\t", 2)
            if len(f) < 2:
                continue
            if f[1] in wanted:
                keep.append(int(f[0][2:]))
            if n % PROGRESS_EVERY == 0:
                print(f"    title.basics: {n:,} rows, {len(keep):,} kept",
                      flush=True)
    out = np.frombuffer(keep, dtype=np.int64)
    print(f"  title.basics: {n:,} rows -> {out.size:,} titles of type "
          f"{sorted(t.decode() for t in wanted)}")
    return out


def read_principals(path, keep_titles, categories):
    if not os.path.isfile(path):
        raise SystemExit(f"missing {path}\nrun download_imdb.sh first")


    if keep_titles is not None:
        keep_sorted = np.sort(keep_titles)
    else:
        keep_sorted = None

    titles = array("q")
    persons = array("q")
    n = 0
    with gzip.open(path, "rb") as fh:
        fh.readline()
        for line in fh:
            n += 1
            f = line.split(b"\t", 4)
            if len(f) < 4:
                continue
            if categories and f[3] not in categories:
                continue
            titles.append(int(f[0][2:]))
            persons.append(int(f[2][2:]))
            if n % PROGRESS_EVERY == 0:
                print(f"    title.principals: {n:,} rows, {len(titles):,} kept",
                      flush=True)

    t = np.frombuffer(titles, dtype=np.int64)
    p = np.frombuffer(persons, dtype=np.int64)
    print(f"  title.principals: {n:,} rows -> {t.size:,} credits after the "
          f"category filter")

    if keep_sorted is not None:

        mask = np.isin(t, keep_sorted, assume_unique=False)
        t, p = t[mask], p[mask]
        print(f"  after --title-types: {t.size:,} credits")

    return t, p


def sort_dedup(rows, cols):
    if rows.size == 0:
        return rows, cols
    order = np.lexsort((cols, rows))
    rows = rows[order]
    cols = cols[order]
    if rows.size > 1:
        fresh = np.empty(rows.size, dtype=bool)
        fresh[0] = True
        np.logical_or(rows[1:] != rows[:-1], cols[1:] != cols[:-1], out=fresh[1:])
        dropped = rows.size - int(fresh.sum())
        rows = rows[fresh]
        cols = cols[fresh]
        if dropped:
            print(f"  deduplicated {dropped:,} repeated (title, person) credits")
    return rows, cols


def degree_filter(ids, other, lo, hi, label):
    if lo <= 1 and hi <= 0:
        return ids, other
    uniq, cnt = np.unique(ids, return_counts=True)
    ok = np.ones(uniq.size, dtype=bool)
    if lo > 1:
        ok &= cnt >= lo
    if hi > 0:
        ok &= cnt <= hi
    keep = uniq[ok]
    mask = np.isin(ids, keep)
    kept_ids, kept_other = ids[mask], other[mask]
    print(f"  {label} degree filter [{lo}, {hi if hi > 0 else 'inf'}]: "
          f"{uniq.size:,} -> {keep.size:,} distinct, "
          f"{ids.size:,} -> {kept_ids.size:,} credits")
    return kept_ids, kept_other


def remap(ids):
    old_of_new, new_ids = np.unique(ids, return_inverse=True)
    return new_ids.astype(np.int64), old_of_new


def write_mtx(path, rows, cols, num_row, num_col):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    nnz = rows.size
    with open(path, "w", encoding="ascii") as fh:
        fh.write("%%MatrixMarket matrix coordinate real general\n")
        fh.write(f"{num_row} {num_col} {nnz}\n")
        for start in range(0, nnz, WRITE_CHUNK):
            chunk_r = (rows[start:start + WRITE_CHUNK] + 1).tolist()
            chunk_c = (cols[start:start + WRITE_CHUNK] + 1).tolist()
            fh.write("".join(
                "%d %d 1.000000\n" % (r, c) for r, c in zip(chunk_r, chunk_c)
            ))
    print(f"  wrote {path}: {num_row:,} x {num_col:,}, nnz={nnz:,} "
          f"({os.path.getsize(path) / 2**30:.2f} GiB)")


def write_bin(path, rows, cols, num_row, num_col):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    nnz = rows.size
    buf = np.empty(nnz + 1, dtype=TRIPLE_DT)
    buf[0]["user_id"] = num_row
    buf[0]["item_id"] = num_col
    buf[0]["rating"] = float(nnz)
    buf[1:]["user_id"] = rows
    buf[1:]["item_id"] = cols
    buf[1:]["rating"] = 1.0
    with open(path, "wb") as fh:
        buf.tofile(fh)
    print(f"  wrote {path}: {num_row:,} x {num_col:,}, nnz={nnz:,} "
          f"({os.path.getsize(path) / 2**30:.2f} GiB)")


def diagnose(rows, cols, num_row, num_col):
    nnz = rows.size
    row_deg = np.bincount(rows, minlength=num_row).astype(np.float64)
    col_deg = np.bincount(cols, minlength=num_col).astype(np.float64)


    flops = float(np.dot(col_deg, col_deg))
    nnz_per_row = nnz / float(num_row)

    print()
    print("--- shape ---")
    print(f"  rows (A axis)      {num_row:,}")
    print(f"  cols (shared axis) {num_col:,}")
    print(f"  nnz                {nnz:,}")
    print(f"  nnzA/row           {nnz_per_row:.2f}   "
          f"(ab>hash>mkl bucket median: 6.0)")
    print(f"  row degree  max {int(row_deg.max()):,}  mean {row_deg.mean():.2f}")
    print(f"  col degree  max {int(col_deg.max()):,}  mean {col_deg.mean():.2f}")
    print(f"  flops(A*A^T)       {flops:,.0f}")
    print(f"    mask-1 sweep ran 3.2e6 .. {FLOP_MAX:.2e} (p90 {FLOP_P90:.2e})")

    if flops > FLOP_WARN:
        print()
        print(f"  WARNING: {flops:.2e} flops exceeds anything in the mask-1 "
              f"sweep by {flops / FLOP_MAX:.1f}x.", file=sys.stderr)
        top = np.sort(col_deg)[-5:][::-1]
        print(f"  The top-5 column degrees are {[int(x) for x in top]} and "
              f"contribute {float(np.dot(top, top)) / flops * 100:.1f}% of it.",
              file=sys.stderr)
        print(f"  Re-run with --max-person-degree "
              f"{int(np.percentile(col_deg[col_deg > 0], 99.9))} or lower, or "
              f"narrow --title-types.", file=sys.stderr)

    if nnz_per_row > 12:
        print()
        print(f"  NOTE: {nnz_per_row:.1f} nnz/row is above the ab>hash>mkl "
              f"bucket (median 6.0, all members <= 18). The mkl>ab>hash bucket "
              f"sits at ~35. Narrowing --categories thins the rows.",
              file=sys.stderr)

    return {
        "num_row": int(num_row),
        "num_col": int(num_col),
        "nnz": int(nnz),
        "nnz_per_row": nnz_per_row,
        "flops_aat": flops,
        "row_deg_max": int(row_deg.max()),
        "col_deg_max": int(col_deg.max()),
    }


def main():
    ap = argparse.ArgumentParser(
        description="IMDb title.principals -> .org/.trs matrix pairs",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("--src", default=DEFAULT_SRC,
                    help="directory holding the IMDb .tsv.gz files (default: %(default)s)")
    ap.add_argument("--name", required=True,
                    help="dataset name, i.e. the argv[1] passed to bin/hashspgemm")
    ap.add_argument("--format", choices=["mtx", "bin", "both"], default="both",
                    help="mtx -> dataset/tamu (IN_TAMU builds); "
                         "bin -> dataset/groupby-sorted (IN_GROUPBY builds); "
                         "default: %(default)s")
    ap.add_argument("--tamu-out", default=TAMU_OUT,
                    help="root for the .mtx pair (default: %(default)s)")
    ap.add_argument("--groupby-out", default=GROUPBY_OUT,
                    help="root for the .bin pair (default: %(default)s)")
    ap.add_argument("--orient", choices=["title", "person"], default="title",
                    help="which axis indexes the rows of A, i.e. what C is a "
                         "similarity matrix over (default: %(default)s)")
    ap.add_argument("--title-types", default="",
                    help="comma-separated titleType values to keep, e.g. "
                         "movie,tvMovie,tvSeries (empty = all; needs title.basics)")
    ap.add_argument("--categories", default="",
                    help="comma-separated principals.category values to keep, "
                         "e.g. actor,actress,director (empty = all)")
    ap.add_argument("--min-person-degree", type=int, default=1,
                    help="drop persons credited on fewer than N kept titles")
    ap.add_argument("--max-person-degree", type=int, default=0,
                    help="drop persons credited on more than N kept titles "
                         "(0 = unlimited); this is the flop-count knob")
    ap.add_argument("--min-title-degree", type=int, default=1,
                    help="drop titles with fewer than N kept credits")
    ap.add_argument("--max-title-degree", type=int, default=0,
                    help="drop titles with more than N kept credits (0 = unlimited)")
    args = ap.parse_args()

    title_types = {t.strip().encode() for t in args.title_types.split(",") if t.strip()}
    categories = {c.strip().encode() for c in args.categories.split(",") if c.strip()}

    print(f"source: {args.src}")
    print("reading...")
    keep_titles = read_title_types(
        os.path.join(args.src, "title.basics.tsv.gz"), title_types)
    t, p = read_principals(
        os.path.join(args.src, "title.principals.tsv.gz"), keep_titles, categories)

    if t.size == 0:
        raise SystemExit("no credits survived the filters")

    print("shaping...")
    t, p = sort_dedup(t, p)


    p, t = degree_filter(p, t, args.min_person_degree, args.max_person_degree, "person")
    t, p = degree_filter(t, p, args.min_title_degree, args.max_title_degree, "title")

    if t.size == 0:
        raise SystemExit("no credits survived the degree filters")

    t, old_t = remap(t)
    p, old_p = remap(p)
    print(f"  renumbered: {old_t.size:,} titles, {old_p.size:,} persons")

    if args.orient == "title":
        rows, cols, num_row, num_col = t, p, old_t.size, old_p.size
    else:
        rows, cols, num_row, num_col = p, t, old_p.size, old_t.size
        rows, cols = sort_dedup(rows, cols)

    stats = diagnose(rows, cols, num_row, num_col)

    print()
    print("writing...")
    if args.format in ("mtx", "both"):
        d = os.path.join(args.tamu_out, args.name)
        write_mtx(os.path.join(d, f"{args.name}.org.mtx"), rows, cols, num_row, num_col)
        write_mtx(os.path.join(d, f"{args.name}.trs.mtx"), cols, rows, num_col, num_row)
    if args.format in ("bin", "both"):
        d = args.groupby_out


        trs_r, trs_c = sort_dedup(cols.copy(), rows.copy())
        write_bin(os.path.join(d, f"{args.name}-sorted.org.bin"),
                  rows, cols, num_row, num_col)
        write_bin(os.path.join(d, f"{args.name}-sorted.trs.bin"),
                  trs_r, trs_c, num_col, num_row)

    meta = {
        "name": args.name,
        "source": args.src,
        "orient": args.orient,
        "format": args.format,
        "filters": {
            "title_types": args.title_types,
            "categories": args.categories,
            "min_person_degree": args.min_person_degree,
            "max_person_degree": args.max_person_degree,
            "min_title_degree": args.min_title_degree,
            "max_title_degree": args.max_title_degree,
        },
        "stats": stats,
    }
    meta_dir = os.path.join(args.tamu_out, args.name) if args.format != "bin" \
        else args.groupby_out
    os.makedirs(meta_dir, exist_ok=True)
    meta_path = os.path.join(meta_dir, f"{args.name}.meta.json")
    with open(meta_path, "w", encoding="utf-8") as fh:
        json.dump(meta, fh, indent=2)
    print(f"  wrote {meta_path}")

    print()
    print("next:")
    print(f"  python scripts/20260207-scripts/imdb_stat.py --name {args.name}")
    print(f"  bash scripts/20260207-scripts/test_imdb_ab.sh")


if __name__ == "__main__":
    main()
