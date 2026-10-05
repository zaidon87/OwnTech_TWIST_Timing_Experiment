#!/usr/bin/env python3
"""Select E0/E0b/E1-E5 by editing only benchmark configuration macros."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

CONFIGS = {
    "E0":  {"TIMING_TEST_MODE": 0, "TEST_SORT_N": 16, "CONTROL_PERIOD_US": 100, "TIMING_IRQ_SOURCE_TIM6": 1},
    "E0B": {"TIMING_TEST_MODE": 2, "TEST_SORT_N": 16, "CONTROL_PERIOD_US": 100, "TIMING_IRQ_SOURCE_TIM6": 1},
    "E1":  {"TIMING_TEST_MODE": 1, "TEST_SORT_N": 4,  "CONTROL_PERIOD_US": 100, "TIMING_IRQ_SOURCE_TIM6": 1},
    "E2":  {"TIMING_TEST_MODE": 1, "TEST_SORT_N": 8,  "CONTROL_PERIOD_US": 100, "TIMING_IRQ_SOURCE_TIM6": 1},
    "E3":  {"TIMING_TEST_MODE": 1, "TEST_SORT_N": 16, "CONTROL_PERIOD_US": 100, "TIMING_IRQ_SOURCE_TIM6": 1},
    "E4":  {"TIMING_TEST_MODE": 1, "TEST_SORT_N": 32, "CONTROL_PERIOD_US": 100, "TIMING_IRQ_SOURCE_TIM6": 1},
    "E5":  {"TIMING_TEST_MODE": 1, "TEST_SORT_N": 64, "CONTROL_PERIOD_US": 100, "TIMING_IRQ_SOURCE_TIM6": 1},
}


def replace_define(text: str, macro: str, value: int) -> str:
    pattern = rf"(?m)^(#define\s+{re.escape(macro)}\s+)(\d+)(U?)\s*$"
    new_text, count = re.subn(pattern, rf"\g<1>{value}\g<3>", text, count=1)
    if count != 1:
        raise RuntimeError(f"Could not uniquely find #define {macro} in main.cpp")
    return new_text


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("experiment", help="E0, E0b, E1, E2, E3, E4, or E5")
    parser.add_argument(
        "--main",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "firmware" / "src" / "main.cpp",
        help="Path to benchmark main.cpp",
    )
    args = parser.parse_args()

    key = args.experiment.upper()
    if key not in CONFIGS:
        raise SystemExit("Supported automatic selections: E0, E0b, E1, E2, E3, E4, E5. E6/E7 require manual integration.")

    path = args.main
    text = path.read_text(encoding="utf-8")
    for macro, value in CONFIGS[key].items():
        text = replace_define(text, macro, value)
    path.write_text(text, encoding="utf-8")

    pretty = "E0b" if key == "E0B" else key
    print(f"Selected {pretty} in {path}")
    for macro, value in CONFIGS[key].items():
        print(f"  {macro} = {value}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
