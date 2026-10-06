import sys
from pathlib import Path
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from analyze_campaign import analyze
from test_timing_analysis import test_parse_and_summary


class CampaignTests(unittest.TestCase):
    def make_log(self, path, n=5):
        lines = []
        for batch in range(1, 6*5*(n+1)*3+1):
            case = (batch-1)//3
            lines.extend([
                f"CVB_META,{batch},{case},0,pattern,1,-1,{case%(n+1)},{n-case%(n+1)},{369*9*(n+1)**2},0,0",
                f"CSV,{batch},3,{n},1000,100,110,120,200,220,240,17,34,51,33990,34000,34010,0,34000",
            ])
        path.write_text("\n".join(lines)+"\n")

    def test_complete_and_repeated_batches(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/"run.log"
            self.make_log(path)
            path.write_text(path.read_text()*2)
            cases, summary = analyze(path)
            self.assertEqual(len(cases), 180)
            self.assertEqual(summary["samples"], 540000)
            self.assertEqual(cases[0]["samples"], 3000)
            self.assertEqual(summary["t_max_cycles"], 120)

    def test_ten_modules_per_arm(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/"run.log"
            self.make_log(path, n=10)
            cases, summary = analyze(path)
            self.assertEqual(len(cases), 330)
            self.assertEqual(summary["samples"], 990000)
            self.assertEqual(summary["selftest_checks"], 401841)
            self.assertEqual(cases[-1]["n_upper"], 10)
            self.assertEqual(cases[-1]["n_lower"], 0)

    def test_reject_incomplete_or_failed_selftest(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/"run.log"
            self.make_log(path)
            complete = path.read_text()
            path.write_text("\n".join(complete.splitlines()[:-2]))
            with self.assertRaisesRegex(ValueError, "Incomplete"):
                analyze(path)
            path.write_text(complete.replace("119556,0,0", "119556,1,0", 1))
            with self.assertRaisesRegex(ValueError, "self-test"):
                analyze(path)

    def test_reject_case_mismatch(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/"run.log"
            self.make_log(path)
            path.write_text(path.read_text().replace("CVB_META,1,0,", "CVB_META,1,1,"))
            with self.assertRaisesRegex(ValueError, "Batch/case"):
                analyze(path)

    def test_upstream_analysis(self):
        test_parse_and_summary()


if __name__ == "__main__":
    unittest.main()
