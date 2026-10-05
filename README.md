# OwnTech_TWIST_Timing Experiment

Reusable timing-analysis repository for critical real-time control tasks on **OwnTech SPIN / TWIST**, based on the reviewed v2 benchmark and its experiment guide.

The repository keeps the timing-critical firmware conservative: `firmware/src/main.cpp` is the reviewed v2 program, unchanged from the supplied source. Analysis, experiment organization, templates, and documentation are separated around it so that the benchmark method is easy to repeat without accidentally changing the measured critical path.

## What it measures

The benchmark combines three complementary timing methods:

1. **DWT_CYCCNT** — internal CPU-cycle timing with 1-cycle resolution.
2. **GPIO pulse + oscilloscope** — external validation of the target window.
3. **TIM6->CNT** — interrupt-entry latency when TIM6 is the trigger, with 0.1 us timer resolution.

It also records the actual entry-to-entry period, period jitter, deadline overruns, worst observed latency-plus-body time, slack, and utilization.

For the reviewed target configuration, the guide uses **STM32G474RE / Cortex-M4F at 170 MHz**, SPIN 1.2.0, TWIST 1.4.2, OwnTech Core, Zephyr RTOS, and PlatformIO/VS Code.

## Safety boundary

**E0 through E6 are USB-only timing experiments.** Do not enable the power stage, PWM outputs, or ADC acquisition for those benchmark runs. **E7 is a separate full-control-loop experiment** and must follow the laboratory's normal power-up, isolation, and probing procedures.

## Repository map

```text
OwnTech_TWIST_Timing_Experiment/
├── README.md
├── QUICK_START.md
├── CHANGELOG.md
├── firmware/
│   ├── src/main.cpp
│   ├── platformio.ini.example
│   ├── zephyr/prj.conf.example
│   └── README.md
├── experiments/
│   ├── README.md
│   ├── E0_instrumentation/
│   ├── E0b_clock_validation/
│   ├── E1_N4/
│   ├── E2_N8/
│   ├── E3_N16/
│   ├── E4_N32/
│   ├── E5_N64/
│   ├── E6_mmc_algorithm/
│   └── E7_full_control_loop/
├── analysis/
│   ├── timing_analysis.py
│   ├── README.md
│   ├── requirements.txt
│   └── tests/
├── results/
│   ├── raw/
│   ├── processed/
│   ├── oscilloscope/
│   └── reports/
├── config/
│   ├── experiment_matrix.csv
│   └── campaign_record.csv
├── docs/
│   ├── OwnTech_TWIST_Timing_Experiment_Guide_EN.md
│   ├── measurement_method.md
│   ├── hardware_wiring.md
│   ├── acceptance_criteria.md
│   └── troubleshooting.md
├── scripts/
│   └── select_experiment.py
└── reference/
    └── main_timing_v2_EN.cpp
```

## Experiment sequence

| Experiment | Firmware configuration | Purpose |
|---|---|---|
| E0 | mode 0 | Instrumentation overhead / scope offset calibration |
| E0b | mode 2, 10 us | CPU-clock cross-check against the oscilloscope |
| E1 | mode 1, N=4 | Bubble-sort baseline |
| E2 | mode 1, N=8 | Bubble-sort baseline |
| E3 | mode 1, N=16 | Bubble-sort baseline |
| E4 | mode 1, N=32 | Bubble-sort baseline |
| E5 | mode 1, N=64 | Bubble-sort stress case |
| E6 | real MMC target | Local sorting / balancing or another real MMC algorithm |
| E7 | full control loop | Acquisition -> control -> sorting/communication -> PWM update |

Use one firmware image per experiment and keep optimization, shield selection, timing source, timing pin, and other campaign settings controlled and recorded.

## Core timing quantities

Let:

- `T` = `task_under_test()` duration measured by DWT.
- `C` = critical callback body duration.
- `L` = interrupt-entry latency from the TIM6 event to the callback entry.
- `Ts` = control period.
- `P` = oscilloscope HIGH-pulse width.

The benchmark uses:

```text
Worst observed ISR-side time = L_max + C_max
Slack                        = Ts - (L_max + C_max)
Utilisation                  = (L_max + C_max) / Ts * 100 %
Scope offset delta           = P_E0 - T_E0
Corrected scope duration     = P - delta
```

The reported maxima are **observed maxima over the collected samples**, not formal static WCET bounds.

## Fastest start

See [QUICK_START.md](QUICK_START.md). In short:

1. Start from an OwnTech project that already builds and uploads an official example.
2. Back up its `src/main.cpp`.
3. Copy `firmware/src/main.cpp` into that project.
4. Keep the documented SPIN/TWIST and optimization settings fixed for a campaign.
5. Wire the oscilloscope to **SPIN pin 9 / PC7** and logic GND for E0-E6.
6. Run E0, then E0b, then E1-E5.
7. Save console logs in `results/raw/` and scope measurements in `results/oscilloscope/`.
8. Run the analysis script to create summaries.
9. Integrate the real MMC function only after the baseline campaign is validated.

## Selecting E0-E5 quickly

The supplied source uses compile-time macros. You can edit them manually, or use:

```bash
python scripts/select_experiment.py E3
```

This changes only the experiment-related macros in `firmware/src/main.cpp`. E6 and E7 remain intentionally manual because they require your actual control function and system initialization.

## Analyze a saved console log

```bash
python analysis/timing_analysis.py \
  results/raw/E3_N16.log \
  --output results/processed/E3_N16_summary.csv
```

If matplotlib is installed, add `--plots-dir results/reports/E3_N16` to create timing plots.

## Hardware timing pin

The reviewed default is:

```text
SPIN header pin 9 -> STM32 PC7 -> oscilloscope CH1
```

Do not start the TIM3 incremental encoder during the benchmark because it can claim PC6/PC7. The guide documents alternate pins if the default becomes unavailable.

## Source-of-truth files

- `firmware/src/main.cpp`: reviewed benchmark implementation used for execution.
- `docs/OwnTech_TWIST_Timing_Experiment_Guide_EN.md`: full English experiment setup guide exported to GitHub-friendly Markdown from the supplied DOCX.
- `reference/main_timing_v2_EN.cpp`: untouched reference copy of the supplied source.

## License

No license has been selected yet. Choose the repository license before publishing it publicly.
