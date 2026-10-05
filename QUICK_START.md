# Quick Start

This is the minimum sequence for a clean E0-E5 timing campaign.

## 1. Verify the OwnTech toolchain

Open an OwnTech Core project in VS Code / PlatformIO that already builds and uploads an official example. Build it once before replacing any source file.

## 2. Install the benchmark source

Back up the project's original `src/main.cpp`, then copy:

```text
firmware/src/main.cpp
```

into the OwnTech project as `src/main.cpp`.

`firmware/platformio.ini.example` and `firmware/zephyr/prj.conf.example` are reference snippets, not guaranteed drop-in replacements for every OwnTech release.

## 3. Keep campaign settings controlled

Recommended reviewed defaults:

```text
CONTROL_PERIOD_US       = 100
TIMING_IRQ_SOURCE_TIM6  = 1
TIMING_GPIO_PIN         = 9
BATCH_SAMPLES           = 1000
TIMING_USE_STM32_LL     = 1
PRINT_CSV               = 1
```

Do not change optimization level between E0 and the experiment that you compare against it.

## 4. Wire the oscilloscope

For E0-E6, use USB power only.

```text
SPIN pin 9 / PC7  ---> oscilloscope CH1 tip
SPIN logic GND    ---> oscilloscope ground
```

Suggested settings from the experiment guide:

```text
Probe       : 10x, DC coupling
CH1         : about 1 V/div
Trigger     : CH1 rising edge, about 1.6 V
E0 timebase : 100-200 ns/div with LL path
E0b         : about 2 us/div
E1-E5       : start around 5 us/div and adapt
Acquisition : normal, no averaging, high sample rate
```

At `Ts = 100 us`, the pulse repetition must be about **10 kHz**.

## 5. Run E0

```bash
python scripts/select_experiment.py E0
```

Build, upload, open the Serial Monitor, and check:

```text
DWT cycle counter: OK
CSV_HEADER,...
```

Record DWT results and scope `+Width` statistics. E0 establishes the measurement overhead and scope correction offset.

## 6. Run E0b

```bash
python scripts/select_experiment.py E0b
```

The target is a DWT-timed 10 us busy-wait. Use the scope as an independent CPU-clock cross-check.

## 7. Run the bubble-sort sweep

```bash
python scripts/select_experiment.py E1
python scripts/select_experiment.py E2
python scripts/select_experiment.py E3
python scripts/select_experiment.py E4
python scripts/select_experiment.py E5
```

Build and upload after each selection. Save each console session separately.

Suggested file names:

```text
results/raw/E0.log
results/raw/E0b.log
results/raw/E1_N4.log
results/raw/E2_N8.log
results/raw/E3_N16.log
results/raw/E4_N32.log
results/raw/E5_N64.log
```

## 8. Analyze a log

```bash
python analysis/timing_analysis.py results/raw/E3_N16.log \
  --output results/processed/E3_N16_summary.csv \
  --plots-dir results/reports/E3_N16
```

The script extracts only lines beginning with `CSV,`, converts cycles to microseconds, and summarizes maxima, jitter, slack, utilization, and overrun status.

## 9. Move to E6 only after the baseline is validated

For E6, replace the body of `task_under_test()` with the actual MMC function. Keep input preparation outside the target window and preserve a visible side effect after the measured region so the compiler cannot remove the computation.

Use difficult but defined input patterns as separate sub-experiments, then report the largest measured maximum.
