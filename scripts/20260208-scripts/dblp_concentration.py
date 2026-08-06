import argparse
import os
import sys

try:
    import numpy as np
except ImportError:
    raise SystemExit("dblp_concentration.py needs numpy: pip install numpy")

try:
    from scipy.sparse import csr_matrix
    HAVE_SCIPY = True
except ImportError:
    HAVE_SCIPY = False

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_OUT = os.path.join(REPO_ROOT, "dataset", "pathsim")


RELATIONS = [("ap", "A", "P"), ("cp", "C", "P"), ("pt", "P", "T")]
NODE_TSV = {"A": "authors.tsv", "P": "papers.tsv", "C": "venues.tsv", "T": "terms.tsv"}
NODE_PLURAL = {"A": "authors", "P": "papers", "C": "venues", "T": "terms"}


HEAD_FRACS = [0.001, 0.01, 0.05, 0.10, 0.20]
MASS_TARGETS = [0.50, 0.80, 0.90, 0.99]


def read_mtx(path):
    if not os.path.isfile(path):
        raise SystemExit(f"missing {path}\nrun dblp2mtx.py first")
    rows = []
    cols = []
    num_row = num_col = nnz = None
    with open(path, "r", encoding="ascii") as fh:
        for line in fh:
            if line.startswith("%"):
                continue
            parts = line.split()
            if not parts:
                continue
            if num_row is None:
                num_row, num_col, nnz = (int(p) for p in parts[:3])
                continue
            rows.append(int(parts[0]) - 1)
            cols.append(int(parts[1]) - 1)
    if num_row is None:
        raise SystemExit(f"{path}: no header line")
    if len(rows) != nnz:
        print(f"  WARNING: {path} header says nnz={nnz}, found {len(rows)}",
              file=sys.stderr)
    return (np.array(rows, dtype=np.int64), np.array(cols, dtype=np.int64),
            num_row, num_col)


def read_names(path, count):
    if not os.path.isfile(path):
        return None
    names = [""] * count
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        for line in fh:
            parts = line.rstrip("\n").rstrip("\r").split("\t")
            if len(parts) < 2:
                continue
            try:
                idx = int(parts[0]) - 1
            except ValueError:
                continue
            if 0 <= idx < count:
                names[idx] = parts[1]
    return names


def resolve_edge(src, trg):
    for stem, rsrc, rtrg in RELATIONS:
        if (rsrc, rtrg) == (src, trg):
            return stem, False
        if (rsrc, rtrg) == (trg, src):
            return stem, True
    raise SystemExit(f"no stored relation for pair {src}{trg}")


def crossover(x):
    x = np.asarray(x, dtype=np.float64)
    n = x.size
    if n == 0:
        return None
    total = float(x.sum())
    if total <= 0:
        return None
    s = np.sort(x)[::-1]
    c = np.cumsum(s)


    k = int(np.searchsorted(c, total / 2.0, side="right")) + 1
    k = min(k, n)
    return {"k": k, "row_frac": k / n, "mass_frac": float(c[k - 1] / total)}


def mass_at_top(x, frac):
    x = np.asarray(x, dtype=np.float64)
    n = x.size
    total = float(x.sum())
    if n == 0 or total <= 0:
        return 0.0
    k = max(1, int(round(n * frac)))
    k = min(k, n)
    s = np.sort(x)[::-1]
    return float(s[:k].sum() / total)


def rows_for_mass(x, target):
    x = np.asarray(x, dtype=np.float64)
    n = x.size
    total = float(x.sum())
    if n == 0 or total <= 0:
        return 0.0
    s = np.sort(x)[::-1]
    c = np.cumsum(s)
    k = int(np.searchsorted(c, target * total, side="left")) + 1
    k = min(k, n)
    return k / n


def gini(x):
    x = np.asarray(x)
    if x.size == 0:
        return 0.0
    total = float(x.sum())
    if total <= 0:
        return 0.0
    s = np.sort(x.astype(np.float64))
    n = s.size
    idx = np.arange(1, n + 1, dtype=np.float64)
    return float((2.0 * np.dot(idx, s)) / (n * total) - (n + 1.0) / n)


def powerlaw_fit(x, min_tail=50, max_candidates=200):
    x = np.asarray(x, dtype=np.float64)
    x = x[x > 0]
    if x.size < min_tail:
        return None
    cand = np.unique(x)
    if cand.size > max_candidates:
        qs = np.linspace(0.0, 95.0, max_candidates)
        cand = np.unique(np.percentile(cand, qs))
    best = None
    for xmin in cand:
        if xmin < 1.0:
            continue
        tail = x[x >= xmin]
        n = tail.size
        if n < min_tail:
            continue
        denom = np.sum(np.log(tail / (xmin - 0.5)))
        if denom <= 0:
            continue
        alpha = 1.0 + n / denom
        if alpha <= 1.0:
            continue
        srt = np.sort(tail)
        cdf_emp = np.arange(1, n + 1, dtype=np.float64) / n
        cdf_fit = 1.0 - (srt / xmin) ** (1.0 - alpha)
        ks = float(np.max(np.abs(cdf_emp - cdf_fit)))
        if best is None or ks < best["ks"]:
            best = {"alpha": float(alpha), "xmin": float(xmin),
                    "ks": ks, "n_tail": int(n)}
    return best


def analyse(x, threads):
    x = np.asarray(x, dtype=np.int64)
    n = x.size
    total = float(x.sum())
    out = {
        "n": n,
        "sum": total,
        "max": int(x.max()) if n else 0,
        "mean": (total / n) if n else 0.0,
        "nonzero": int((x > 0).sum()),
        "gini": gini(x),
    }
    cx = crossover(x)
    if cx:
        out["crossover_k"] = cx["k"]
        out["crossover_row_frac"] = cx["row_frac"]
        out["crossover_mass_frac"] = cx["mass_frac"]
    else:
        out["crossover_k"] = 0
        out["crossover_row_frac"] = 0.0
        out["crossover_mass_frac"] = 0.0
    for f in HEAD_FRACS:
        out[f"top{f*100:g}pct_share"] = mass_at_top(x, f)
    for m in MASS_TARGETS:
        out[f"rows_for_{int(m*100)}pct"] = rows_for_mass(x, m)


    if n and total > 0 and threads >= 1:
        fair = total / threads
        out["thread_fair_share"] = fair
        out["hot_row_vs_thread"] = out["max"] / fair if fair > 0 else 0.0
        out["rows_for_one_thread"] = rows_for_mass(x, 1.0 / threads)
    else:
        out["thread_fair_share"] = 0.0
        out["hot_row_vs_thread"] = 0.0
        out["rows_for_one_thread"] = 0.0

    fit = powerlaw_fit(x)
    if fit:
        out["alpha"] = fit["alpha"]
        out["alpha_xmin"] = fit["xmin"]
        out["alpha_ks"] = fit["ks"]
        out["alpha_ntail"] = fit["n_tail"]
    return out


def sentence(st, row_plural, mass_label, dataset):
    if not st.get("crossover_k"):
        return None
    rf = st["crossover_row_frac"]
    return (f"On {dataset}, the top {rf*100:.2f}% of {row_plural} account for more "
            f"{mass_label} than the remaining {100-rf*100:.2f}% combined "
            f"({st['crossover_mass_frac']*100:.1f}% of all {mass_label}, "
            f"{st['crossover_k']:,} of {st['n']:,} rows).")


def print_block(label, st, unit, threads):
    print(f"\n  {label}")
    print(f"    rows={st['n']:,}  total {unit}={int(st['sum']):,}  "
          f"nonzero rows={st['nonzero']:,}  max={st['max']:,}  "
          f"mean={st['mean']:,.2f}")
    if st.get("crossover_k"):
        print(f"    CROSSOVER: top {st['crossover_row_frac']*100:.3f}% of rows "
              f"({st['crossover_k']:,}) hold {st['crossover_mass_frac']*100:.1f}% "
              f"-> more than the other {100-st['crossover_row_frac']*100:.3f}% combined")
    head = "  ".join(f"top{f*100:g}%={st[f'top{f*100:g}pct_share']*100:.1f}%"
                     for f in HEAD_FRACS)
    print(f"    head shares: {head}")
    inv = "  ".join(f"{int(m*100)}%<-{st[f'rows_for_{int(m*100)}pct']*100:.3f}% of rows"
                    for m in MASS_TARGETS)
    print(f"    mass targets: {inv}")
    print(f"    Gini={st['gini']:.3f}")
    if "alpha" in st:
        print(f"    power-law tail: alpha={st['alpha']:.2f}  xmin={st['alpha_xmin']:,.0f}  "
              f"KS={st['alpha_ks']:.3f}  n_tail={st['alpha_ntail']:,}  "
              f"(descriptive only, no GoF test)")
    print(f"    {threads} threads: fair share={st['thread_fair_share']:,.0f} {unit}; "
          f"hottest row = {st['hot_row_vs_thread']:.3f}x one thread's share; "
          f"{st['rows_for_one_thread']*100:.4f}% of rows fill one thread")
    if st["hot_row_vs_thread"] > 1.0:
        print(f"      -> a SINGLE row exceeds one thread's budget: no row-wise "
              f"partition can balance this, row splitting is required")
    else:
        print(f"      -> every row fits inside one thread's budget, so a "
              f"row-wise partition CAN be balanced (this bounds what Adaptive "
              f"Binning could recover)")


def print_heavy(x, names, node_type, top):
    if top <= 0:
        return
    x = np.asarray(x)
    order = np.argsort(-x)[:top]
    total = float(x.sum())
    print(f"    heaviest {top}:")
    for rank, i in enumerate(order, start=1):
        nm = names[i] if names else ""
        share = (x[i] / total * 100.0) if total > 0 else 0.0
        print(f"      {rank:2d}. {node_type}={i}\t{int(x[i]):,}\t({share:.2f}%)\t{nm}")


def collect(args):
    data_dir = os.path.join(args.out, args.name)
    if not os.path.isdir(data_dir):
        raise SystemExit(f"missing {data_dir}\n"
                         f"run dblp2mtx.py --name {args.name} first")

    print(f"dataset: {data_dir}")
    print(f"threads assumed for the load bound: {args.threads}")
    if not HAVE_SCIPY:
        print("  (scipy absent -- M_l row-nnz block and multi-step chains skipped)")

    rows = []
    quotes = []
    series = []


    print("\n" + "=" * 74)
    print("RELATION DEGREE CONCENTRATION")
    print("=" * 74)

    for stem, src_type, trg_type in RELATIONS:
        path = os.path.join(data_dir, f"{stem}.org.mtx")
        if not os.path.isfile(path):
            print(f"\n  {stem}: not present, skipping")
            continue
        r, c, num_row, num_col = read_mtx(path)
        print(f"\n{stem}.org.mtx  ({src_type} x {trg_type})  "
              f"{num_row:,} x {num_col:,}  nnz={r.size:,}")

        for axis_idx, (axis, node_type, count) in enumerate(
                ((r, src_type, num_row), (c, trg_type, num_col))):
            deg = np.bincount(axis, minlength=count).astype(np.int64)
            direction = (f"{src_type}->{trg_type}" if axis_idx == 0
                         else f"{trg_type}->{src_type}")
            st = analyse(deg, args.threads)
            print_block(f"degree of {node_type} ({direction})", st,
                        "edges", args.threads)
            names = read_names(os.path.join(data_dir, NODE_TSV[node_type]), count)
            print_heavy(deg, names, node_type, args.top)

            mass_label = f"{src_type}-{trg_type} edges"
            q = sentence(st, NODE_PLURAL[node_type], mass_label, args.name)
            if q:
                quotes.append(q)
            rows.append({"scope": "degree",
                         "what": f"{stem}:{node_type}:{direction}", **st})
            series.append((f"deg {node_type} ({direction})", deg))


    for mp in [m.strip() for m in args.metapath.split(",") if m.strip()]:
        if len(mp) < 3 or len(mp) % 2 == 0 or mp != mp[::-1]:
            raise SystemExit(f"--metapath {mp}: must be palindromic, odd length >= 3")
        num_edge = (len(mp) - 1) // 2

        print("\n" + "=" * 74)
        print(f"M_l SpGEMM ROW-FLOP CONCENTRATION  --  {mp}, left half {mp[:num_edge+1]}")
        print("=" * 74)

        operands = []
        for i in range(num_edge):
            stem, transposed = resolve_edge(mp[i], mp[i + 1])
            p = os.path.join(data_dir, f"{stem}.{'trs' if transposed else 'org'}.mtx")
            rr, cc, nr, nc = read_mtx(p)
            operands.append((os.path.basename(p), rr, cc, nr, nc))
            print(f"\n  operand {i+1}/{num_edge}: {os.path.basename(p)}  "
                  f"{nr:,} x {nc:,}  nnz={rr.size:,}")

        if num_edge == 1:
            print(f"\n  {mp} has a single left-half edge: M_l IS that operand, no "
                  f"SpGEMM runs. Its row-nnz concentration is the "
                  f"'degree of {mp[0]}' block above.")
            continue

        a_name, a_rows, a_cols, a_nrow, _ = operands[0]
        acc = None
        skipped_product = False
        for i in range(1, num_edge):
            b_name, b_r, b_c, b_nr, b_nc = operands[i]


            b_deg = np.bincount(b_r, minlength=b_nr).astype(np.int64)
            flop = np.bincount(a_rows, weights=b_deg[a_cols],
                               minlength=a_nrow).astype(np.int64)

            print(f"\n  step {i}/{num_edge-1}: {a_name} x {b_name}  "
                  f"-> {a_nrow:,} x {b_nc:,}")
            st = analyse(flop, args.threads)
            print_block("per-row flop (intermediate products)", st,
                        "flop", args.threads)
            names = (read_names(os.path.join(data_dir, NODE_TSV[mp[0]]), a_nrow)
                     if i == 1 else None)
            print_heavy(flop, names, mp[0] if i == 1 else "row", args.top)

            if i == 1:
                q = sentence(st, NODE_PLURAL[mp[0]],
                             f"{mp} SpGEMM flop", args.name)
                if q:
                    quotes.append(q)
            rows.append({"scope": "spgemm_flop",
                         "what": f"{mp} step{i}: {a_name} x {b_name}", **st})
            series.append((f"{mp} row flop", flop))

            if not HAVE_SCIPY:
                print("    (no scipy: chain analysis stops after step 1)")
                break


            step_flop = float(st["sum"])
            if 0 < args.max_product_flop < step_flop:
                print(f"    (skipping the M_l product: {step_flop:,.0f} flop exceeds "
                      f"--max-product-flop {args.max_product_flop:,.0f}.")
                print(f"     The row-flop concentration above is unaffected; only "
                      f"the M_l row-nnz block is lost. Raise the limit to force it.)")
                if i + 1 < num_edge:
                    print(f"    [ABORT] {mp} needs this product to continue the chain")
                skipped_product = True
                break

            a_mat = csr_matrix((np.ones(a_rows.size), (a_rows, a_cols)),
                               shape=(a_nrow, b_nr))
            b_mat = csr_matrix((np.ones(b_r.size), (b_r, b_c)), shape=(b_nr, b_nc))
            prod = (a_mat @ b_mat).tocsr()
            acc = prod


            if i + 1 < num_edge:
                out_nnz = np.diff(prod.indptr).astype(np.int64)
                a_rows = np.repeat(np.arange(prod.shape[0], dtype=np.int64),
                                   out_nnz)
                a_cols = prod.indices.astype(np.int64)
                a_nrow = prod.shape[0]
                a_name = "acc"

        if HAVE_SCIPY and acc is not None and not skipped_product:
            ml_nnz = np.diff(acc.indptr).astype(np.int64)
            print(f"\n  M_l: {acc.shape[0]:,} x {acc.shape[1]:,}  nnz={acc.nnz:,}")
            st = analyse(ml_nnz, args.threads)
            print_block("M_l row nnz (drives the per-query SpMV and diag)",
                        st, "nnz", args.threads)
            rows.append({"scope": "m_l_nnz", "what": f"M_l:{mp}", **st})
            series.append((f"{mp} M_l row nnz", ml_nnz))

    return rows, quotes, series


CSV_KEYS = (["scope", "what", "n", "sum", "max", "mean", "nonzero", "gini",
             "crossover_k", "crossover_row_frac", "crossover_mass_frac"]
            + [f"top{f*100:g}pct_share" for f in HEAD_FRACS]
            + [f"rows_for_{int(m*100)}pct" for m in MASS_TARGETS]
            + ["thread_fair_share", "hot_row_vs_thread", "rows_for_one_thread",
               "alpha", "alpha_xmin", "alpha_ks", "alpha_ntail"])


def write_csv(path, rows):
    import csv
    with open(path, "w", newline="", encoding="utf-8") as fh:
        w = csv.DictWriter(fh, fieldnames=CSV_KEYS, extrasaction="ignore")
        w.writeheader()
        for row in rows:
            w.writerow(row)
    print(f"\nwrote {path} ({len(rows)} rows)")


def write_md(path, rows, dataset, threads):
    out_dir = os.path.dirname(path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(f"### Concentration — `{dataset}` ({threads} threads)\n\n")
        fh.write("| Distribution | rows | total | crossover (top q% > rest) "
                 "| top-1% | top-10% | Gini | alpha | hottest row / thread share |\n")
        fh.write("|---|---:|---:|---:|---:|---:|---:|---:|---:|\n")
        for r in rows:
            alpha = f"{r['alpha']:.2f}" if "alpha" in r else "—"
            fh.write(
                f"| `{r['what']}` | {r['n']:,} | {int(r['sum']):,} "
                f"| **{r['crossover_row_frac']*100:.2f}%** "
                f"| {r['top1pct_share']*100:.1f}% "
                f"| {r['top10pct_share']*100:.1f}% "
                f"| {r['gini']:.3f} | {alpha} "
                f"| {r['hot_row_vs_thread']:.3f}× |\n")
        fh.write("\nCrossover = smallest head of the distribution holding strictly "
                 "more mass than the entire remaining tail.\n")
        fh.write("`hottest row / thread share` > 1.0 means no row-wise partition "
                 "can be balanced; < 1.0 bounds what row-wise load balancing "
                 "could recover.\n")
    print(f"wrote {path}")


def make_plot(prefix, series, dataset):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("  (--plot needs matplotlib: pip install matplotlib)", file=sys.stderr)
        return

    out_dir = os.path.dirname(prefix)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    colors = ["#4C6EF5", "#F59F00", "#0CA678", "#E8590C",
              "#7048E8", "#1098AD", "#D6336C", "#5C940D"]
    styles = ["-", "--", "-.", ":"]
    fig, (ax0, ax1) = plt.subplots(1, 2, figsize=(11.0, 4.4))

    for i, (label, x) in enumerate(series):
        x = np.asarray(x, dtype=np.float64)
        total = x.sum()
        if x.size == 0 or total <= 0:
            continue
        col = colors[i % len(colors)]
        sty = styles[(i // len(colors)) % len(styles)]


        s = np.sort(x)[::-1]
        cum = np.cumsum(s) / total
        frac = np.arange(1, s.size + 1, dtype=np.float64) / s.size
        ax0.plot(frac * 100.0, cum * 100.0, label=label,
                 color=col, linestyle=sty, linewidth=1.6)
        cx = crossover(x)
        if cx:
            ax0.plot([cx["row_frac"] * 100.0], [cx["mass_frac"] * 100.0],
                     marker="o", markersize=4.5, color=col, linestyle="none")


        pos = s[s > 0]
        if pos.size:
            rank = np.arange(1, pos.size + 1, dtype=np.float64) / pos.size
            ax1.plot(pos, rank, label=label, color=col,
                     linestyle=sty, linewidth=1.6)

    ax0.axhline(50.0, color="#888888", linewidth=0.8, linestyle=":")
    ax0.set_xscale("log")
    ax0.set_xlabel("heaviest x% of rows (log)")
    ax0.set_ylabel("% of total mass held")
    ax0.set_title(f"{dataset}: concentration (dot = crossover)")
    ax0.set_ylim(0, 100)

    ax1.set_xscale("log")
    ax1.set_yscale("log")
    ax1.set_xlabel("value per row (degree / flop / nnz)")
    ax1.set_ylabel("fraction of rows at least this large")
    ax1.set_title(f"{dataset}: tail")

    for ax in (ax0, ax1):
        ax.grid(True, which="major", linewidth=0.4, color="#CCCCCC")
        ax.grid(True, which="minor", linewidth=0.25, color="#EEEEEE")
        ax.set_axisbelow(True)
        for spine in ("top", "right"):
            ax.spines[spine].set_visible(False)
    ax0.legend(frameon=False, fontsize=7, loc="lower right")
    fig.tight_layout()

    for ext in ("png", "eps"):
        path = f"{prefix}.{ext}"
        fig.savefig(path, format=ext, dpi=200)
        print(f"  wrote {path}")
    plt.close(fig)


def selftest():
    ok = 0

    def check(name, got, want, tol=0.0):
        nonlocal ok
        good = abs(got - want) <= tol if tol else got == want
        print(f"  [{'PASS' if good else 'FAIL'}] {name}: got {got}, want {want}")
        if good:
            ok += 1
        else:
            raise SystemExit(f"selftest failed at: {name}")

    print("crossover()")


    check("tie needs a second row", crossover([10] + [1] * 10)["k"], 2)
    check("no tie wins at k=1", crossover([10] + [1] * 9)["k"], 1)

    check("uniform n=1000", crossover([1] * 1000)["k"], 501)

    check("single hot row", crossover([1000] + [0] * 999)["k"], 1)
    check("single hot row frac", crossover([1000] + [0] * 999)["row_frac"],
          0.001, 1e-12)
    check("empty is None", crossover([]) is None, True)
    check("all-zero is None", crossover([0, 0, 0]) is None, True)

    print("mass_at_top() / rows_for_mass()")
    x = [10] + [1] * 10
    check("top 10% share", mass_at_top(x, 0.10), 0.5, 1e-12)
    check("rows for 50%", rows_for_mass(x, 0.50), 1.0 / 11.0, 1e-12)
    check("rows for 100%", rows_for_mass(x, 1.0), 1.0, 1e-12)

    print("gini()")
    check("uniform gini ~ 0", gini(np.ones(1000)), 0.0, 1e-9)
    g = gini(np.array([1000] + [0] * 999))
    check("degenerate gini ~ 1", g > 0.998, True)

    print("row-flop formula (matches get_intprod in src/mm/mm_utils.c)")


    a_rows = np.array([0, 0, 1]);  a_cols = np.array([0, 1, 2])
    b_rows = np.array([0, 1, 1, 2]); b_cols = np.array([0, 1, 0, 1])
    b_deg = np.bincount(b_rows, minlength=3).astype(np.int64)
    flop = np.bincount(a_rows, weights=b_deg[a_cols], minlength=2).astype(np.int64)
    check("flop[0]", int(flop[0]), 3)
    check("flop[1]", int(flop[1]), 1)

    print("thread bound")

    st = analyse(np.array([500] + [500 // 99] * 99), threads=10)
    check("hot row exceeds thread budget", st["hot_row_vs_thread"] > 1.0, True)
    st = analyse(np.ones(1000, dtype=np.int64), threads=10)
    check("uniform hot row far below budget",
          st["hot_row_vs_thread"] < 0.02, True)

    print("powerlaw_fit() on a synthetic Pareto(alpha=2.5)")
    rng = np.random.default_rng(12345)
    samp = np.floor((rng.pareto(1.5, 200000) + 1) * 10).astype(np.int64)
    fit = powerlaw_fit(samp)
    check("fit returned", fit is not None, True)


    check("alpha near 2.5", abs(fit["alpha"] - 2.5) < 0.35, True)

    print(f"\nAll {ok} checks passed.")


def main():
    ap = argparse.ArgumentParser(
        description="concentration / crossover report for a pathsim dataset")
    ap.add_argument("--out", default=DEFAULT_OUT, help="dataset/pathsim root")
    ap.add_argument("--name", default="", help="dataset subdirectory name")
    ap.add_argument("--metapath", default="",
                    help="comma-separated meta paths, e.g. APCPA,APTPA,TPAPT")
    ap.add_argument("--threads", type=int, default=64,
                    help="thread count for the load bound (default 64)")
    ap.add_argument("--top", type=int, default=10,
                    help="list the N heaviest rows per distribution (0 = skip)")
    ap.add_argument("--max-product-flop", type=float, default=2e8,
                    help="skip materializing M_l for steps above this flop count "
                         "(default 2e8; 0 disables the guard). The row-flop "
                         "concentration is computed either way.")
    ap.add_argument("--csv", default="", help="write every statistic to this CSV")
    ap.add_argument("--md", default="", help="write a markdown table to this path")
    ap.add_argument("--plot", default="",
                    help="figure prefix; writes <prefix>.png and <prefix>.eps")
    ap.add_argument("--selftest", action="store_true",
                    help="verify the statistics on known inputs and exit")
    args = ap.parse_args()

    if args.selftest:
        selftest()
        return
    if not args.name:
        raise SystemExit("--name is required (or use --selftest)")

    rows, quotes, series = collect(args)

    if quotes:
        print("\n" + "=" * 74)
        print("QUOTABLE — crossover claims, in the shape of the PageRank example")
        print("=" * 74)
        for q in quotes:
            print(f"  * {q}")

    if args.csv and rows:
        write_csv(args.csv, rows)
    if args.md and rows:
        write_md(args.md, rows, args.name, args.threads)
    if args.plot and series:
        make_plot(args.plot, series, args.name)

    print("\nReading the numbers:")
    print("  The crossover is the claim; Gini and the head shares are context.")
    print("  A crossover under ~5% is a strong, quotable concentration result;")
    print("  near 50% means the distribution is effectively flat.")
    print("  hot_row_vs_thread is the operational number: above 1.0 no row-wise")
    print("  partition can balance, below 1.0 it can, and the gap to 1.0 is the")
    print("  headroom any row-wise load balancer (Adaptive Binning included)")
    print("  has to work with. See scripts/20260208-scripts/README.md, where AB")
    print("  came out 2-8% SLOWER than plain hash on all three meta paths.")
    print("  Note that concentration is NOT the same as partition imbalance:")
    print("  row ORDER also matters, and on DBLP author ids correlate with")
    print("  degree, so heavy rows cluster rather than spread.")


if __name__ == "__main__":
    main()
