#!/usr/bin/env python3
"""Parse OwnTech SPIN/TWIST timing benchmark CSV lines and summarize a run."""

from __future__ import annotations

import argparse
import csv
import math
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List

FIELDS = [
    "batch", "mode", "N", "samples",
    "t_min", "t_avg", "t_max",
    "c_min", "c_avg", "c_max",
    "l_min", "l_avg", "l_max",
    "p_min", "p_avg", "p_max",
    "overruns", "budget_cycles",
]


@dataclass(frozen=True)
class Row:
    batch: int
    mode: int
    N: int
    samples: int
    t_min: int
    t_avg: int
    t_max: int
    c_min: int
    c_avg: int
    c_max: int
    l_min: int
    l_avg: int
    l_max: int
    p_min: int
    p_avg: int
    p_max: int
    overruns: int
    budget_cycles: int


def parse_rows(lines: Iterable[str]) -> List[Row]:
    rows: List[Row] = []
    for raw in lines:
        line = raw.strip()
        if not line.startswith("CSV,"):
            continue
        parts = line.split(",")
        if len(parts) != 19:
            raise ValueError(
                f"Expected 19 comma-separated fields including 'CSV', got {len(parts)}: {line}"
            )
        values = [int(x.strip()) for x in parts[1:]]
        rows.append(Row(*values))
    if not rows:
        raise ValueError("No firmware lines beginning with 'CSV,' were found.")
    return rows


def cycles_to_us(cycles: float, cpu_hz: float) -> float:
    return cycles * 1_000_000.0 / cpu_hz


def summarize(rows: List[Row], cpu_hz: float) -> dict:
    first = rows[0]
    if any(r.mode != first.mode or r.N != first.N for r in rows):
        raise ValueError("Input contains more than one firmware mode/N configuration. Split the log first.")
    if any(r.budget_cycles != first.budget_cycles for r in rows):
        raise ValueError("Input contains more than one timing budget. Split the log first.")

    total_samples = sum(r.samples for r in rows)
    total_overruns = sum(r.overruns for r in rows)

    t_min = min(r.t_min for r in rows)
    t_max = max(r.t_max for r in rows)
    c_min = min(r.c_min for r in rows)
    c_max = max(r.c_max for r in rows)
    l_min = min(r.l_min for r in rows)
    l_max = max(r.l_max for r in rows)
    p_min = min(r.p_min for r in rows)
    p_max = max(r.p_max for r in rows)

    # Weighted averages preserve batch sample counts if BATCH_SAMPLES changes.
    def wavg(attr: str) -> float:
        return sum(getattr(r, attr) * r.samples for r in rows) / total_samples

    t_avg = wavg("t_avg")
    c_avg = wavg("c_avg")
    l_avg = wavg("l_avg")
    p_avg = wavg("p_avg")

    worst_cycles = l_max + c_max
    budget = first.budget_cycles
    slack_cycles = budget - worst_cycles
    utilization_pct = (worst_cycles / budget * 100.0) if budget else math.nan
    period_jitter_cycles = p_max - p_min
    passed = total_overruns == 0 and worst_cycles < budget

    return {
        "mode": first.mode,
        "N": first.N,
        "batches": len(rows),
        "samples": total_samples,
        "cpu_hz": int(cpu_hz),
        "budget_cycles": budget,
        "budget_us": cycles_to_us(budget, cpu_hz),
        "t_min_cycles": t_min,
        "t_avg_cycles": t_avg,
        "t_max_cycles": t_max,
        "t_min_us": cycles_to_us(t_min, cpu_hz),
        "t_avg_us": cycles_to_us(t_avg, cpu_hz),
        "t_max_us": cycles_to_us(t_max, cpu_hz),
        "c_min_cycles": c_min,
        "c_avg_cycles": c_avg,
        "c_max_cycles": c_max,
        "c_min_us": cycles_to_us(c_min, cpu_hz),
        "c_avg_us": cycles_to_us(c_avg, cpu_hz),
        "c_max_us": cycles_to_us(c_max, cpu_hz),
        "l_min_cycles": l_min,
        "l_avg_cycles": l_avg,
        "l_max_cycles": l_max,
        "l_min_us": cycles_to_us(l_min, cpu_hz),
        "l_avg_us": cycles_to_us(l_avg, cpu_hz),
        "l_max_us": cycles_to_us(l_max, cpu_hz),
        "period_min_cycles": p_min,
        "period_avg_cycles": p_avg,
        "period_max_cycles": p_max,
        "period_avg_us": cycles_to_us(p_avg, cpu_hz),
        "period_jitter_cycles": period_jitter_cycles,
        "period_jitter_us": cycles_to_us(period_jitter_cycles, cpu_hz),
        "worst_latency_plus_body_cycles": worst_cycles,
        "worst_latency_plus_body_us": cycles_to_us(worst_cycles, cpu_hz),
        "slack_cycles": slack_cycles,
        "slack_us": cycles_to_us(slack_cycles, cpu_hz),
        "utilization_pct": utilization_pct,
        "overruns": total_overruns,
        "deadline": "PASS" if passed else "FAIL",
    }


def write_summary(summary: dict, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["metric", "value"])
        for key, value in summary.items():
            if isinstance(value, float):
                writer.writerow([key, f"{value:.6f}"])
            else:
                writer.writerow([key, value])


def make_plots(rows: List[Row], cpu_hz: float, out_dir: Path) -> None:
    try:
        import matplotlib.pyplot as plt
    except ImportError as exc:
        raise SystemExit("Plotting requested but matplotlib is not installed. Run: pip install -r analysis/requirements.txt") from exc

    out_dir.mkdir(parents=True, exist_ok=True)
    batches = [r.batch for r in rows]

    def us(values):
        return [cycles_to_us(v, cpu_hz) for v in values]

    plt.figure()
    plt.plot(batches, us([r.t_max for r in rows]), marker="o")
    plt.xlabel("Batch")
    plt.ylabel("Target max [us]")
    plt.title("Observed target maximum by batch")
    plt.tight_layout()
    plt.savefig(out_dir / "target_max_by_batch.png", dpi=160)
    plt.close()

    plt.figure()
    plt.plot(batches, us([r.c_max for r in rows]), marker="o", label="Critical body max")
    plt.plot(batches, us([r.l_max + r.c_max for r in rows]), marker="o", label="Latency + body max")
    plt.xlabel("Batch")
    plt.ylabel("Time [us]")
    plt.title("Critical timing by batch")
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / "critical_timing_by_batch.png", dpi=160)
    plt.close()

    plt.figure()
    plt.plot(batches, us([r.p_max - r.p_min for r in rows]), marker="o")
    plt.xlabel("Batch")
    plt.ylabel("Period jitter [us]")
    plt.title("Period jitter by batch")
    plt.tight_layout()
    plt.savefig(out_dir / "period_jitter_by_batch.png", dpi=160)
    plt.close()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path, help="Serial Monitor log containing firmware CSV lines")
    parser.add_argument("--cpu-hz", type=float, default=170_000_000.0, help="CPU frequency used to convert cycles to time")
    parser.add_argument("--output", type=Path, help="Write two-column summary CSV")
    parser.add_argument("--plots-dir", type=Path, help="Optional directory for PNG plots")
    args = parser.parse_args()

    with args.log.open("r", encoding="utf-8", errors="replace") as f:
        rows = parse_rows(f)

    summary = summarize(rows, args.cpu_hz)

    print("OwnTech TWIST timing summary")
    print(f"  Mode / N               : {summary['mode']} / {summary['N']}")
    print(f"  Batches / samples      : {summary['batches']} / {summary['samples']}")
    print(f"  Target max             : {summary['t_max_us']:.3f} us")
    print(f"  Critical body max      : {summary['c_max_us']:.3f} us")
    print(f"  Entry latency max      : {summary['l_max_us']:.3f} us")
    print(f"  Latency + body max     : {summary['worst_latency_plus_body_us']:.3f} us")
    print(f"  Period jitter          : {summary['period_jitter_us']:.3f} us")
    print(f"  Slack                  : {summary['slack_us']:.3f} us")
    print(f"  Utilization            : {summary['utilization_pct']:.2f} %")
    print(f"  Overruns               : {summary['overruns']}")
    print(f"  Deadline               : {summary['deadline']}")

    if args.output:
        write_summary(summary, args.output)
        print(f"  Summary CSV            : {args.output}")
    if args.plots_dir:
        make_plots(rows, args.cpu_hz, args.plots_dir)
        print(f"  Plots                  : {args.plots_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
