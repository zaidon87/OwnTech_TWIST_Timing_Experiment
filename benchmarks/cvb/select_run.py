"""Select a compile-time calibration or CVB scope; does not flash the board."""
import argparse
from pathlib import Path
import re

parser = argparse.ArgumentParser(__doc__)
parser.add_argument("mode", type=int, choices=(0, 2, 3))
parser.add_argument("--scope", type=int, choices=(0, 1, 2), default=0)
args = parser.parse_args()
config = Path(__file__).resolve().parents[2] / "src" / "timing_config.h"
text = config.read_text()
for name, value in (("TIMING_TEST_MODE", args.mode), ("CVB_SCOPE", args.scope)):
    text, count = re.subn(rf"(#define\s+{name}\s+)\d+", rf"\g<1>{value}", text)
    assert count == 1, name
config.write_text(text)
print(f"Selected mode={args.mode}, scope={args.scope}")
