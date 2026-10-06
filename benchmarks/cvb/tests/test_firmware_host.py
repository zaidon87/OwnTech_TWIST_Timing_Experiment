"""Run the actual CVB headers on the host; requires clang++ or g++."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

SOURCE = Path(__file__).resolve().parents[3] / "src"
COMPILER = shutil.which("clang++") or shutil.which("g++")


@unittest.skipUnless(COMPILER, "A host C++ compiler is required")
class FirmwareTests(unittest.TestCase):
    def test_supported_sizes(self):
        for n in (5, 10, 20, 30):
            with self.subTest(n=n), tempfile.TemporaryDirectory() as directory:
                work = Path(directory)
                for name in ("cvb_cases.h", "cvb_algorithm.h", "timing_config.h"):
                    shutil.copyfile(SOURCE/name, work/name)
                config = work/"timing_config.h"
                config.write_text(re.sub(r"(#define\s+CVB_MODULES_PER_ARM\s+)\d+U",
                                         rf"\g<1>{n}U", config.read_text()))
                (work/"arm_math_types.h").write_text("typedef float float32_t;\n")
                (work/"test.cpp").write_text(r'''#include "cvb_cases.h"
#include <assert.h>
#include <stdio.h>
int main() {
    for (uint32_t a = 0; a < 243; ++a) {
        float x[CVB_N], y[CVB_N];
        for (unsigned i = 0; i < CVB_N; ++i) { x[i] = -100; y[i] = 999; }
        cvb_make_tied_pattern(a, x);
        cvb_make_tied_pattern(a, y);
        unsigned code = a;
        for (unsigned i = 0; i < CVB_N; ++i) {
            assert(x[i] == y[i]);
            assert(x[i] >= 70 && x[i] <= 72);
            if (i < 5) { assert(x[i] == 70 + code % 3); code /= 3; }
            else { assert(x[i] == 70 + (unsigned(x[4-i%5]-70) + a%3)%3); }
        }
    }
    memset(g_u, 0, sizeof(g_u)); memset(g_l, 0, sizeof(g_l));
    for (unsigned bit = 0; bit < 2*CVB_N; ++bit) {
        uint8_t *gate = bit < CVB_N ? &g_u[bit] : &g_l[bit-CVB_N];
        *gate = 1; consume_cvb_result();
        assert(cvb_result_guard == (uint64_t(1) << bit));
        *gate = 0;
    }
    memset(g_u, 1, sizeof(g_u)); memset(g_l, 1, sizeof(g_l));
    consume_cvb_result();
    assert(cvb_result_guard == (uint64_t(1) << (2*CVB_N))-1);
    assert(cvb_selftest());
    printf("N=%u checks=%u failures=%u\n", CVB_N, cvb_selftest_checks, cvb_selftest_failures);
}
''')
                executable = work/("test.exe" if os.name == "nt" else "test")
                subprocess.run([COMPILER, "-std=c++17", "-O2", str(work/"test.cpp"),
                                "-o", str(executable)], check=True, capture_output=True, text=True)
                result = subprocess.run([str(executable)], check=True, capture_output=True,
                                        text=True, timeout=180)
                print(result.stdout.strip())


if __name__ == "__main__":
    unittest.main()
