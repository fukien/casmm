import json
import os
import re
from collections import Counter

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
INPUT_DIR = os.path.join(REPO_ROOT, "dataset", "dblp", "dblp-ref-v10")
OUTPUT_DIR = os.path.join(REPO_ROOT, "dataset", "dblp", "data_dblp_v10")
MIN_TERM_DF = 2

STOPWORDS = {
    "a", "an", "the", "of", "in", "on", "at", "to", "for", "with", "by",
    "from", "as", "and", "or", "but", "is", "are", "was", "were", "be",
    "been", "being", "this", "that", "these", "those", "it", "its", "we",
    "our", "you", "your", "they", "their", "he", "she", "his", "her",
    "not", "no", "so", "than", "then", "if", "into", "via", "using",
    "based", "towards", "toward",
}

TOKEN_RE = re.compile(r"[a-z]+")


def tokenize(title: str) -> set:
    tokens = TOKEN_RE.findall(title.lower())
    return {t for t in tokens if len(t) >= 2 and t not in STOPWORDS}


def iter_records(dir_path: str):
    if not os.path.isdir(dir_path):
        raise SystemExit(
            f"missing {dir_path}\n"
            "run scripts/20260208-scripts/fetch_dblp_v10.sh first"
        )
    files = sorted(f for f in os.listdir(dir_path) if f.endswith(".json"))
    if not files:
        raise SystemExit(f"no .json shards in {dir_path}")
    for fname in files:
        path = os.path.join(dir_path, fname)
        print(f"  reading {path}")
        with open(path, "r", encoding="utf-8") as fh:
            for line in fh:
                line = line.strip()
                if not line:
                    continue
                try:
                    yield json.loads(line)
                except json.JSONDecodeError:
                    continue


def sanitize(s: str) -> str:
    return s.replace("\t", " ").replace("\n", " ").replace("\r", " ").strip()


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    print("Pass 1/2: computing term document frequencies...")
    term_df = Counter()
    n_records = 0
    for rec in iter_records(INPUT_DIR):
        title = rec.get("title") or ""
        if title:
            for t in tokenize(title):
                term_df[t] += 1
        n_records += 1
        if n_records % 500_000 == 0:
            print(f"    {n_records:,} records scanned, {len(term_df):,} unique terms")
    print(f"  total records: {n_records:,}")
    print(f"  unique terms before DF filter: {len(term_df):,}")

    term_ids = {}
    for term in sorted(term_df):
        if term_df[term] >= MIN_TERM_DF:
            term_ids[term] = len(term_ids) + 1
    print(f"  terms kept (DF >= {MIN_TERM_DF}): {len(term_ids):,}")

    print("Pass 2/2: emitting nodes and relations...")
    author_ids = {}
    venue_ids = {}

    paths = {name: os.path.join(OUTPUT_DIR, name + ".tsv") for name in
             ["authors", "papers", "venues", "terms", "ap", "cp", "pt"]}
    fhs = {k: open(v, "w", encoding="utf-8") for k, v in paths.items()}

    for term, tid in term_ids.items():
        fhs["terms"].write(f"{tid}\t{term}\n")

    paper_counter = 0
    ap_edges = cp_edges = pt_edges = 0
    for rec in iter_records(INPUT_DIR):
        paper_counter += 1
        pid = paper_counter
        uuid = sanitize(rec.get("id") or "")
        title = sanitize(rec.get("title") or "")
        fhs["papers"].write(f"{pid}\t{uuid}\t{title}\n")

        seen_authors = set()
        for name in (rec.get("authors") or []):
            name = sanitize(name or "")
            if not name or name in seen_authors:
                continue
            seen_authors.add(name)
            aid = author_ids.get(name)
            if aid is None:
                aid = len(author_ids) + 1
                author_ids[name] = aid
                fhs["authors"].write(f"{aid}\t{name}\n")
            fhs["ap"].write(f"{aid}\t{pid}\n")
            ap_edges += 1

        venue = sanitize(rec.get("venue") or "")
        if venue:
            cid = venue_ids.get(venue)
            if cid is None:
                cid = len(venue_ids) + 1
                venue_ids[venue] = cid
                fhs["venues"].write(f"{cid}\t{venue}\n")
            fhs["cp"].write(f"{cid}\t{pid}\n")
            cp_edges += 1

        if title:
            for t in tokenize(title):
                tid = term_ids.get(t)
                if tid is not None:
                    fhs["pt"].write(f"{pid}\t{tid}\n")
                    pt_edges += 1

        if paper_counter % 500_000 == 0:
            print(f"    {paper_counter:,} papers emitted")

    for fh in fhs.values():
        fh.close()

    print("Done.")
    print(f"  authors: {len(author_ids):,}")
    print(f"  papers:  {paper_counter:,}")
    print(f"  venues:  {len(venue_ids):,}")
    print(f"  terms:   {len(term_ids):,}")
    print(f"  A-P edges: {ap_edges:,}")
    print(f"  C-P edges: {cp_edges:,}")
    print(f"  P-T edges: {pt_edges:,}")
    print(f"Output in: {OUTPUT_DIR}/")
    print("Next: python scripts/20260208-scripts/dblp2mtx.py "
          "--name dblp_v10_top20c --top-venues 20")


if __name__ == "__main__":
    main()
