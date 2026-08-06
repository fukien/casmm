import argparse
import json
import os
import sys
from array import array

try:
    import numpy as np
except ImportError:
    raise SystemExit("dblp2mtx.py needs numpy: pip install numpy")

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_SRC = os.path.join(REPO_ROOT, "dataset", "dblp", "data_dblp_v10")
DEFAULT_OUT = os.path.join(REPO_ROOT, "dataset", "pathsim")

WRITE_CHUNK = 1 << 20


RELATIONS = [
    ("ap", "A", "P"),
    ("cp", "C", "P"),
    ("pt", "P", "T"),
]

NODE_TSV = {"A": "authors.tsv", "P": "papers.tsv", "C": "venues.tsv", "T": "terms.tsv"}


def read_pairs(path):
    if not os.path.isfile(path):
        raise SystemExit(f"missing {path}\nrun prep_dblp_v10.py first")
    left = array("q")
    right = array("q")
    with open(path, "rb") as fh:
        for line in fh:
            tab = line.find(b"\t")
            if tab < 0:
                continue
            left.append(int(line[:tab]))
            right.append(int(line[tab + 1:]))
    a = np.frombuffer(left, dtype=np.int64)
    b = np.frombuffer(right, dtype=np.int64)
    print(f"  read {path}: {a.size:,} edges")
    return a, b


def read_names(path):
    if not os.path.isfile(path):
        return {}
    names = {}
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        for line in fh:
            line = line.rstrip("\n").rstrip("\r")
            parts = line.split("\t")
            if len(parts) < 2:
                continue
            try:
                names[int(parts[0])] = parts[1]
            except ValueError:
                continue
    return names


def count_lines(path):
    if not os.path.isfile(path):
        raise SystemExit(f"missing {path}\nrun prep_dblp_v10.py first")
    n = 0
    with open(path, "rb") as fh:
        for line in fh:
            if line.strip():
                n += 1
    return n


def keep_mask(ids, max_id, keep_set):
    lut = np.zeros(max_id + 1, dtype=bool)
    if len(keep_set):
        lut[np.fromiter(keep_set, dtype=np.int64, count=len(keep_set))] = True
    return lut[ids]


def top_by_degree(ids, k):
    uniq, cnt = np.unique(ids, return_counts=True)
    if k >= uniq.size:
        return set(uniq.tolist())
    order = np.argsort(-cnt, kind="stable")[:k]
    return set(uniq[order].tolist())


def degree_at_least(ids, n):
    uniq, cnt = np.unique(ids, return_counts=True)
    return set(uniq[cnt >= n].tolist())


def write_mtx(path, rows, cols, num_row, num_col):
    if rows.size:
        order = np.lexsort((cols, rows))
        rows = rows[order]
        cols = cols[order]

        if rows.size > 1:
            fresh = np.empty(rows.size, dtype=bool)
            fresh[0] = True
            np.logical_or(rows[1:] != rows[:-1], cols[1:] != cols[:-1], out=fresh[1:])
            rows = rows[fresh]
            cols = cols[fresh]

    nnz = rows.size
    with open(path, "w", encoding="ascii") as fh:
        fh.write("%%MatrixMarket matrix coordinate real general\n")
        fh.write(f"{num_row} {num_col} {nnz}\n")
        for start in range(0, nnz, WRITE_CHUNK):
            chunk_r = rows[start:start + WRITE_CHUNK].tolist()
            chunk_c = cols[start:start + WRITE_CHUNK].tolist()
            fh.write("".join(
                "%d %d 1.000000\n" % (r, c) for r, c in zip(chunk_r, chunk_c)
            ))
    print(f"  wrote {path}: {num_row} x {num_col}, nnz={nnz:,}")
    return nnz


def write_names(path, names, old_of_new):
    with open(path, "w", encoding="utf-8") as fh:
        for new_id, old_id in enumerate(old_of_new, start=1):
            fh.write(f"{new_id}\t{names.get(int(old_id), '')}\n")
    print(f"  wrote {path}: {len(old_of_new):,} names")


def remap(ids, old_of_new, max_old):
    lut = np.zeros(max_old + 1, dtype=np.int64)
    lut[old_of_new] = np.arange(1, old_of_new.size + 1, dtype=np.int64)
    return lut[ids]


def main():
    ap = argparse.ArgumentParser(
        description="DBLP HIN TSVs -> dataset/pathsim/<name>/*.{org,trs}.mtx",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("--src", default=DEFAULT_SRC,
                    help="directory holding the 7 TSVs (default: %(default)s)")
    ap.add_argument("--out", default=DEFAULT_OUT,
                    help="dataset/pathsim root (default: %(default)s)")
    ap.add_argument("--name", required=True,
                    help="subdirectory name, i.e. the <dataset> argument to bin/*_pathsim")
    ap.add_argument("--top-venues", type=int, default=0,
                    help="keep only the N venues with the most papers (0 = all)")
    ap.add_argument("--venues", default="",
                    help="comma-separated venue names to keep instead of --top-venues")
    ap.add_argument("--min-author-papers", type=int, default=0,
                    help="drop authors with fewer than N kept papers")
    ap.add_argument("--max-authors", type=int, default=0,
                    help="keep only the N authors with the most kept papers (0 = all)")
    ap.add_argument("--min-term-df", type=int, default=0,
                    help="drop terms appearing in fewer than N kept papers")
    args = ap.parse_args()

    out_dir = os.path.join(args.out, args.name)
    os.makedirs(out_dir, exist_ok=True)
    print(f"source: {args.src}")
    print(f"target: {out_dir}")

    n_a = count_lines(os.path.join(args.src, "authors.tsv"))
    n_p = count_lines(os.path.join(args.src, "papers.tsv"))
    n_c = count_lines(os.path.join(args.src, "venues.tsv"))
    n_t = count_lines(os.path.join(args.src, "terms.tsv"))
    print(f"full graph: |A|={n_a:,} |P|={n_p:,} |C|={n_c:,} |T|={n_t:,}")

    print("reading relations...")
    ap_a, ap_p = read_pairs(os.path.join(args.src, "ap.tsv"))
    cp_c, cp_p = read_pairs(os.path.join(args.src, "cp.tsv"))
    pt_p, pt_t = read_pairs(os.path.join(args.src, "pt.tsv"))

    venue_names = read_names(os.path.join(args.src, "venues.tsv"))


    print("subsetting...")
    if args.venues:
        wanted = {v.strip() for v in args.venues.split(",") if v.strip()}
        keep_c = {cid for cid, name in venue_names.items() if name in wanted}
        missing = wanted - {venue_names[c] for c in keep_c}
        if missing:
            print(f"  WARNING: venue names not found: {sorted(missing)}", file=sys.stderr)
        print(f"  venues by name: {len(keep_c):,}")
    elif args.top_venues > 0:
        keep_c = top_by_degree(cp_c, args.top_venues)
        print(f"  top-{args.top_venues} venues by paper count: {len(keep_c):,}")
    else:
        keep_c = None

    if keep_c is not None:
        mask = keep_mask(cp_c, n_c, keep_c)
        cp_c, cp_p = cp_c[mask], cp_p[mask]
        keep_p = set(np.unique(cp_p).tolist())
        print(f"  papers reachable from kept venues: {len(keep_p):,}")
    else:
        keep_p = None


    if keep_p is not None:
        mask = keep_mask(ap_p, n_p, keep_p)
        ap_a, ap_p = ap_a[mask], ap_p[mask]

    if args.min_author_papers > 1:
        keep_a = degree_at_least(ap_a, args.min_author_papers)
        print(f"  authors with >= {args.min_author_papers} kept papers: {len(keep_a):,}")
    else:
        keep_a = None

    if args.max_authors > 0:
        by_degree = top_by_degree(ap_a, args.max_authors)
        keep_a = by_degree if keep_a is None else (keep_a & by_degree)
        print(f"  after --max-authors {args.max_authors}: {len(keep_a):,}")

    if keep_a is not None:
        mask = keep_mask(ap_a, n_a, keep_a)
        ap_a, ap_p = ap_a[mask], ap_p[mask]

        keep_p = set(np.unique(ap_p).tolist())
        print(f"  papers with >= 1 kept author: {len(keep_p):,}")
        mask = keep_mask(cp_p, n_p, keep_p)
        cp_c, cp_p = cp_c[mask], cp_p[mask]


    if keep_p is not None:
        mask = keep_mask(pt_p, n_p, keep_p)
        pt_p, pt_t = pt_p[mask], pt_t[mask]

    if args.min_term_df > 1:
        keep_t = degree_at_least(pt_t, args.min_term_df)
        print(f"  terms with DF >= {args.min_term_df} in kept papers: {len(keep_t):,}")
        mask = keep_mask(pt_t, n_t, keep_t)
        pt_p, pt_t = pt_p[mask], pt_t[mask]


    old_a = np.unique(ap_a) if ap_a.size else np.zeros(0, dtype=np.int64)
    old_p = np.unique(np.concatenate([ap_p, cp_p, pt_p])) if (
        ap_p.size or cp_p.size or pt_p.size) else np.zeros(0, dtype=np.int64)
    old_c = np.unique(cp_c) if cp_c.size else np.zeros(0, dtype=np.int64)
    old_t = np.unique(pt_t) if pt_t.size else np.zeros(0, dtype=np.int64)

    print(f"emitted subset: |A|={old_a.size:,} |P|={old_p.size:,} "
          f"|C|={old_c.size:,} |T|={old_t.size:,}")

    ap_a = remap(ap_a, old_a, n_a)
    ap_p = remap(ap_p, old_p, n_p)
    cp_c = remap(cp_c, old_c, n_c)
    cp_p = remap(cp_p, old_p, n_p)
    pt_p = remap(pt_p, old_p, n_p)
    pt_t = remap(pt_t, old_t, n_t)

    counts = {"A": old_a.size, "P": old_p.size, "C": old_c.size, "T": old_t.size}
    edges = {
        "ap": (ap_a, ap_p),
        "cp": (cp_c, cp_p),
        "pt": (pt_p, pt_t),
    }


    print("writing matrices...")
    nnz = {}
    for stem, src_type, trg_type in RELATIONS:
        rows, cols = edges[stem]
        nnz[stem] = write_mtx(
            os.path.join(out_dir, f"{stem}.org.mtx"),
            rows, cols, counts[src_type], counts[trg_type])
        write_mtx(
            os.path.join(out_dir, f"{stem}.trs.mtx"),
            cols, rows, counts[trg_type], counts[src_type])


    print("writing names...")
    for node_type, old_ids in (("A", old_a), ("P", old_p), ("C", old_c), ("T", old_t)):
        tsv = NODE_TSV[node_type]
        write_names(os.path.join(out_dir, tsv),
                    read_names(os.path.join(args.src, tsv)), old_ids)

    meta = {
        "name": args.name,
        "source": args.src,
        "filters": {
            "top_venues": args.top_venues,
            "venues": args.venues,
            "min_author_papers": args.min_author_papers,
            "max_authors": args.max_authors,
            "min_term_df": args.min_term_df,
        },
        "nodes": counts,
        "edges": nnz,
    }
    with open(os.path.join(out_dir, "meta.json"), "w", encoding="utf-8") as fh:
        json.dump(meta, fh, indent=2)

    print(f"\ndone: {out_dir}")
    print(f"run:   bin/mkl_dcsrmultcsr_pathsim {args.name} APCPA 0 10")
    print(f"check: python scripts/20260208-scripts/dblp_concentration.py --name {args.name}")


if __name__ == "__main__":
    main()
