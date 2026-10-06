# E1: centralized CVB, ten modules per arm

E1 is prepared and compiled for **10 modules per arm, 20 total**. It has not
been flashed or timed on the board in this update. Existing N=5 measurements
remain unchanged under `results/` and do not describe E1 performance.

## Configuration

| Setting | E1 value |
| --- | --- |
| `CVB_MODULES_PER_ARM` | 10 |
| `TIMING_TEST_MODE` | 3 (actual CVB, not mode 1's generic bubble sort) |
| `CVB_SCOPE` | 0 (both arms) |
| Period | 200 us |
| Samples per batch | 1000 |
| Cases | 330: six patterns, five current pairs, eleven insertion-count pairs |
| Batches per case | 3 |
| Full campaign | 990 batches / 990,000 executions, nominally 198 seconds |
| Startup correctness checks | 401,841 combinations; timing starts only on PASS |

The original sorting functions still perform N+1 full passes with N-1 comparisons
per pass: 99 comparisons per arm for E1. The target includes index reset, sorting
and gate selection. Input preparation and output consumption remain outside it.
Output masks now use ten bits per arm, with lower-arm bits 10..19.

The startup test uses six patterns, 243 bounded tied vectors, and 120 seeded
shuffled permutations. It checks all nine current-sign pairs and all 121
upper/lower count pairs. This is deliberately **not** exhaustive enumeration of
10! permutations or 3^10 tied vectors. N=5 mode retains its original exhaustive
test set and patterns; switching sizes requires recompilation.

## Execute from the current project root

Close other serial monitors before upload or capture. Upload through the existing
OwnTech USB bootloader using PlatformIO; the following command replaces the
firmware currently running on the attached board:

```powershell
$pio = 'C:\Users\Dell\.platformio\penv\Scripts\pio.exe'
$python = 'C:\Users\Dell\.platformio\penv\Scripts\python.exe'
& $pio run -e USB -t upload
& $python benchmarks/cvb/capture.py benchmarks/cvb/results/E1_N10_both.log --mode 3 --scope 0 --modules-per-arm 10 --timeout 300
& $python benchmarks/cvb/analyze_campaign.py benchmarks/cvb/results/E1_N10_both.log --output-dir benchmarks/cvb/results/processed
```

The capture script accepts only N=10 records with 401841 self-test checks and zero
failures. Wait for all 990 distinct batch phases. Use a new output filename for
each repeat. The board initializes neither PWM/power outputs nor ADC acquisition.
For a live console instead of capture, use `pio device monitor -e USB` (do not run
two serial readers simultaneously). The startup line should report
`CVB_SELFTEST,PASS,checks=401841,failures=0`; every `CVB_META` report also includes
the check count and failures. The CSV N field should be 10.

## E0 before E1 scope calibration

For an E0 image with the same N=10 source setup:

```powershell
& $python benchmarks/cvb/select_run.py 0
& $pio run -e USB -t upload
& $python benchmarks/cvb/capture.py benchmarks/cvb/results/E0_N10.log --mode 0 --modules-per-arm 10
```

Record the PC7 positive pulse width on the oscilloscope to determine the offset.
Then restore E1 and upload it before collecting E1 data:

```powershell
& $python benchmarks/cvb/select_run.py 3 --scope 0
& $pio run -e USB -t upload
```

Selecting a mode does not change the module count. For the per-arm E1 campaigns,
select mode 3 with `--scope 1` or `--scope 2`, upload, and pass the matching scope
and `--modules-per-arm 10` to capture. Use distinct filenames.

## Version package

`versions/E1_N10.zip` contains the five application source files, this guide,
capture/analysis tools, the signed MCUboot image, and build hashes. It is an
application snapshot for this OwnTech Core project, not a standalone framework.
To restore it into a compatible OwnTech Core project, copy its five `.cpp`/`.h`
application files into `src/` and its Python helpers into `benchmarks/cvb/`.
Run the commands from that project's root, not from inside the extracted archive.
This current workspace is already configured; no copying is needed here.
The regular PlatformIO USB upload command above is the supported upload path;
do not flash the signed image at an arbitrary address.

Build validation: USB build succeeded, 112008 bytes flash and 34273 bytes RAM.
Five host analysis tests passed, including a full synthetic N=10 campaign.
E1 startup correctness checks and execution times still require board execution.
