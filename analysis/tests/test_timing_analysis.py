from pathlib import Path
import sys

ANALYSIS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ANALYSIS_DIR))

import timing_analysis as mod


def test_parse_and_summary():
    lines = [
        "noise\n",
        "CSV,1,1,16,1000,100,120,150,130,160,200,17,34,51,16990,17000,17010,0,17000\n",
        "CSV,2,1,16,1000,101,121,155,131,161,205,17,34,68,16980,17001,17020,0,17000\n",
    ]
    rows = mod.parse_rows(lines)
    s = mod.summarize(rows, 170_000_000.0)
    assert len(rows) == 2
    assert s["t_max_cycles"] == 155
    assert s["c_max_cycles"] == 205
    assert s["l_max_cycles"] == 68
    assert s["period_jitter_cycles"] == 40
    assert s["overruns"] == 0
    assert s["deadline"] == "PASS"
