"""Record source/firmware hashes for the image just built or flashed."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(__doc__)
parser.add_argument("run")
parser.add_argument("--date", default="2026-10-05")
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
files = ["src/main.cpp", "src/cvb_algorithm.h", "src/cvb_cases.h",
         "src/timing_harness.h", "src/timing_config.h",
         ".pio/build/USB/firmware.elf", ".pio/build/USB/firmware.mcuboot.bin"]
out = root/"benchmarks/cvb/results/builds"
out.mkdir(parents=True, exist_ok=True)
config = (root/"src/timing_config.h").read_text()
meta = {
    "run": args.run, "date": args.date,
    "source_baseline": "ecb51f5c807ef0e06e5c59e451165c63c6d227b5",
    "head_at_build": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip(),
    "note": "Source hashes identify the exact working-tree inputs, including any uncommitted changes.",
    "cpu_hz": 170000000, "optimization": "CONFIG_SIZE_OPTIMIZATIONS=y (-Os)",
    "compiler": "arm-none-eabi GCC 12.3.1", "zephyr": "4.0.0",
    "board": "spin@1_2_0", "shield_build": "twist_v1_4_1",
    "board_serial": "423250070032003B", "scope_validation": "not performed",
    "sha256": {f: hashlib.sha256((root/f).read_bytes()).hexdigest() for f in files},
}
(out/f"{args.run}.json").write_text(json.dumps(meta, indent=2)+"\n")
(out/f"{args.run}_config.h").write_text(config)
print(f"Recorded build metadata for {args.run}")
