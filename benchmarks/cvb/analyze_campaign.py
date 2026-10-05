"""Split a complete CVB log by case, reusing the unmodified timing-v2 reader."""
import argparse
import csv
import json
from pathlib import Path

from timing_analysis import parse_rows, summarize


def analyze(path, cpu_hz=170000000):
    lines = path.read_text(encoding="utf-8").splitlines()
    metadata = {}
    selected = {}
    scope = None
    for line in lines:
        if line.startswith("CVB_META,"):
            fields = line.split(",")
            if len(fields) != 12:
                raise ValueError("Invalid metadata record")
            metadata[int(fields[1])] = fields
        elif line.startswith("CSV,"):
            row = parse_rows([line])[0]
            meta = metadata.pop(row.batch, None)
            if meta is None:
                continue  # Capture can begin in the middle of the first report.
            if row.mode != 3 or row.N != 5 or row.samples != 1000:
                raise ValueError("Not a five-module CVB campaign")
            current_scope = int(meta[3])
            if scope is not None and scope != current_scope:
                raise ValueError("Mixed CVB scopes")
            scope = current_scope
            if int(meta[9]) != 119556 or int(meta[10]):
                raise ValueError("Failed or missing self-test")
            phase = (row.batch - 1) % 540
            if int(meta[2]) != phase // 3:
                raise ValueError("Batch/case mismatch")
            selected.setdefault(phase, (row, meta))
    if len(selected) != 540:
        raise ValueError(f"Incomplete campaign: {len(selected)}/540 batch phases")
    cases = []
    for case in range(180):
        entries = [selected[case*3+i] for i in range(3)]
        meta = entries[0][1]
        result = {
            "scope": scope, "case": case, "pattern": meta[4],
            "i_upper": int(meta[5]), "i_lower": int(meta[6]),
            "n_upper": int(meta[7]), "n_lower": int(meta[8]),
        }
        if any(e[1][2:9] != meta[2:9] for e in entries):
            raise ValueError("Inconsistent metadata within case")
        result.update(summarize([e[0] for e in entries], cpu_hz))
        cases.append(result)
    overall = summarize([selected[i][0] for i in sorted(selected)], cpu_hz)
    overall.update(scope=scope, cases=180, selftest_checks=119556,
                   selftest_failures=0, oscilloscope_validated=False,
                   maximum_case=max(cases, key=lambda c: c["t_max_cycles"])["case"])
    return cases, overall


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--cpu-hz", type=int, default=170000000)
    args = parser.parse_args()
    cases, overall = analyze(args.log, args.cpu_hz)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    base = args.output_dir / args.log.stem
    with base.with_suffix(".cases.csv").open("w", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=list(cases[0]))
        writer.writeheader()
        writer.writerows(cases)
    base.with_suffix(".summary.json").write_text(json.dumps(overall, indent=2)+"\n")
    print(json.dumps(overall, indent=2))


if __name__ == "__main__":
    main()
