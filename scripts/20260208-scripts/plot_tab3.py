import argparse
import csv
import glob
import math
import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))
DEFAULT_LOGS = os.path.join(REPO_ROOT, "logs", "tab3-logs")

BACKENDS = [("AMS-HASH", "ab_hashspgemm_pathsim"),
            ("MKL", "mkl_dcsrmultcsr_pathsim")]

METAPATHS = ["APCPA", "APTPA", "TPAPT", "CPAPC"]

SPGEMM_PARTS = ("ml_init", "ml_symb", "ml_mgmt", "ml_numc")

ANSI = re.compile(r"\x1b\[[0-9;]*m")


def parse_summary(path):
    fields = None
    with open(path, encoding="utf-8", errors="replace") as fh:
        for line in fh:
            line = ANSI.sub("", line).strip()
            if not line.startswith("backend:"):
                continue
            parts = [p for p in line.split("\t") if p.strip()]
            kv = {}
            for part in parts:
                if ":" not in part:
                    continue
                key, _, val = part.partition(":")
                kv[key.strip()] = val.strip()
            if "ml_numc" in kv:
                fields = kv
    return fields


def spgemm_seconds(kv):
    return sum(float(kv[k]) for k in SPGEMM_PARTS)


def load_timings(log_dir):
    out = {mp: {} for mp in METAPATHS}
    for display, stem in BACKENDS:
        for mp in METAPATHS:
            pattern = os.path.join(log_dir, f"*_{stem}_{mp}_q*.log")
            matches = sorted(glob.glob(pattern))
            if not matches:
                continue
            if len(matches) > 1:
                print(f"note: {len(matches)} logs match {os.path.basename(pattern)}; "
                      f"using {os.path.basename(matches[-1])}", file=sys.stderr)
            kv = parse_summary(matches[-1])
            if kv is None:
                print(f"warning: no summary line in {matches[-1]} — skipping",
                      file=sys.stderr)
                continue
            out[mp][display] = spgemm_seconds(kv)
    return out


def load_shape(log_dir):
    matches = sorted(glob.glob(os.path.join(log_dir, "*_concentration.csv")))
    if not matches:
        return {}
    shape = {}
    with open(matches[-1], newline="", encoding="utf-8") as fh:
        for row in csv.DictReader(fh):
            if row.get("scope") != "m_l_nnz":
                continue
            mp = row["what"].split(":", 1)[1]
            if mp in METAPATHS:
                shape[mp] = (int(float(row["n"])), int(float(row["sum"])),
                             float(row["gini"]))
    return shape


def human(n):
    if n >= 1_000_000:
        return f"{n / 1_000_000:.2f}M"
    if n >= 1_000:
        return f"{n / 1_000:.2f}K"
    return str(n)


def geomean(vals):
    return math.exp(sum(math.log(v) for v in vals) / len(vals)) if vals else None


def build_rows(timings, shape):
    rows = []
    for mp in METAPATHS:
        t = timings.get(mp, {})
        rows.append({
            "metapath": mp,
            "rows": shape.get(mp, (None,) * 3)[0],
            "nnz": shape.get(mp, (None,) * 3)[1],
            "gini": shape.get(mp, (None,) * 3)[2],
            "ams": t.get("AMS-HASH"),
            "mkl": t.get("MKL"),
        })
    return rows


def fmt(rows):
    head = ("| Meta-path | \\|row\\| | \\|nnz\\| | Gini | AMS-HASH | MKL | Speedup |\n"
            "|---|---:|---:|---:|---:|---:|---:|\n")
    body = []
    for r in rows:
        cells = [
            r["metapath"],
            human(r["rows"]) if r["rows"] is not None else "--",
            human(r["nnz"]) if r["nnz"] is not None else "--",
            f"{r['gini']:.2f}" if r["gini"] is not None else "--",
            f"{r['ams']:.3f}" if r["ams"] is not None else "--",
            f"{r['mkl']:.3f}" if r["mkl"] is not None else "--",
            f"{r['mkl'] / r['ams']:.2f}x" if r["ams"] and r["mkl"] else "--",
        ]
        body.append("| " + " | ".join(cells) + " |\n")

    paired = [r for r in rows if r["ams"] and r["mkl"]]
    if paired:
        g_ams = geomean([r["ams"] for r in paired])
        g_mkl = geomean([r["mkl"] for r in paired])
        body.append(f"| Geomean | -- | -- | -- | {g_ams:.3f} | {g_mkl:.3f} "
                    f"| {g_mkl / g_ams:.2f}x |\n")
        if len(paired) < len(rows):
            done = {r["metapath"] for r in paired}
            missing = [r["metapath"] for r in rows if r["metapath"] not in done]
            body.append(f"\n> Geomean covers {len(paired)}/{len(rows)} meta paths; "
                        f"missing timings for {', '.join(missing)}.\n")
    return head + "".join(body)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--logs", default=DEFAULT_LOGS)
    ap.add_argument("--md", default="")
    args = ap.parse_args()

    if not os.path.isdir(args.logs):
        raise SystemExit(f"no such log directory: {args.logs}\n"
                         f"run scripts/20260208-scripts/run_tab3.sh first")

    timings = load_timings(args.logs)
    shape = load_shape(args.logs)
    if not shape:
        print(f"warning: no *_concentration.csv in {args.logs}; "
              f"shape and Gini columns will be blank", file=sys.stderr)

    table = fmt(build_rows(timings, shape))
    print("\nTable 3: M_l construction on DBLP V10 (seconds).\n")
    print(table)

    if args.md:
        os.makedirs(os.path.dirname(os.path.abspath(args.md)), exist_ok=True)
        with open(args.md, "w", encoding="utf-8") as fh:
            fh.write("### Table 3: $M_l$ construction on DBLP V10 (seconds)\n\n")
            fh.write(table)
        print(f"wrote {args.md}")


if __name__ == "__main__":
    main()
