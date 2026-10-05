# CVB timing results - 2026-10-05

Centralized CVB from `hackathon_lille/main` at `ecb51f5`, five modules per arm.
Measured on SPIN USB serial `423250070032003B` using GCC ARM 12.3.1, Zephyr
4.0.0, size optimization (-Os), configured CPU clock 170 MHz, and a 200 us TIM6
period. Power outputs, PWM, ADC and RS485 were not initialized.

## Measured target

The target includes the original index reset, bubble sort and insertion-selection
code. Input construction/copies, output consumption and reporting are outside.
No algorithm optimization was applied. The combined result measures both calls
in one window; it should not be replaced by a sum of separate-arm maxima.

| Target | Samples | Minimum (us) | Average (us) | Observed maximum (us) | Maximum (cycles) |
| --- | ---: | ---: | ---: | ---: | ---: |
| Both arms | 540,000 | 8.700 | 8.929 | 9.312 | 1583 |
| Upper arm | 540,000 | 4.371 | 4.496 | 4.724 | 803 |
| Lower arm | 540,000 | 4.453 | 4.560 | 4.759 | 809 |

Each target covers 180 input cases, three batches of 1000 executions per case.
The largest observed maximum occurred with descending voltages and both currents
negative (case 36 is the first such case attaining the maximum). Batch averages
are integer cycles; aggregate averages inherit that truncation. Raw instrumentation
is included; no empty-window maximum has been subtracted.

## Maximum target duration by voltage pattern

| Pattern | Both arms (us) | Upper arm (us) | Lower arm (us) |
| --- | ---: | ---: | ---: |
| ascending | 8.782 | 4.459 | 4.494 |
| descending | 9.312 | 4.724 | 4.759 |
| equal | 8.782 | 4.459 | 4.494 |
| ties | 8.900 | 4.518 | 4.553 |
| alternating | 9.018 | 4.576 | 4.612 |
| random | 9.076 | 4.606 | 4.641 |

## Validation

- On-board independent correctness reference: 119,556 combinations passed at
  each startup, including all 5! distinct orderings, all 3^5 tied arrangements,
  both current signs and zero, and all upper/lower insertion counts.
- Host verification: every reported output mask in the three campaign logs
  matched the independent ordering/selection reference (see output_validation.json).
- Capture: all 540 batch phases present per target; no reporting gaps in accepted logs.
- Python analysis tests: four passed. Optimized assembly retains the intended
  upper/lower calls; assembly excerpts are saved in builds/.
- E0: 20,000 empty-window samples, min 6 / average 8 / max 17 cycles
  (0.035 / 0.047 / 0.100 us).
- E0b: 20,000 samples of nominal 10 us DWT delay, min 10.200 / average 10.245 /
  max 10.429 us including wrapper/read overhead. This is an internal reference
  check; **no oscilloscope or independent clock validation was performed**.
- Final board state: mode 3 / scope 0 (both arms) restored and five live batches
  verified. Its ELF SHA-256 exactly matches the measured combined-arm ELF.

## Scheduling diagnostics and limits

| Target | Body maximum (us) | Entry-latency maximum (us) | Period jitter, max-min (us) | Body overruns |
| --- | ---: | ---: | ---: | ---: |
| Both arms | 13.488 | 3.700 | 0.694 | 0 |
| Upper arm | 8.965 | 3.600 | 14.959 | 0 |
| Lower arm | 8.935 | 3.600 | 0.700 | 0 |

The upper-only capture includes the startup batch (case 0): its maximum period
was 214.606 us and minimum TIM6 entry-latency reading was zero. That transient
is retained in the reported period jitter. Combined/lower captures began after
startup. The maximum upper CVB target occurred in case 36, not that startup case.

The timing-v2 callback-body window excludes later statistics/publication and ISR
exit. No body overruns were observed, but this does not prove a full production
control-loop deadline. These are observed maxima for the recorded inputs and
builds, not formal WCET bounds. The production application executes CVB only
when the upper-arm insertion count changes; these tests force active calls.
Input restoration also warms the working data before each measured call.

## Files and provenance

- Raw runs: E0.log, E0b.log, CVB_both.log, CVB_upper.log, CVB_lower.log.
- processed/: per-case CSV, per-target JSON and calibration summaries.
- builds/: configuration snapshots, source/ELF/signed-image hashes and assembly.
- final_smoke.log / final_smoke.json: verification after restoring combined mode.
- diagnostics/: rejected initial E0 captures, retained separately. Verbose output
  overflowed Zephyr's logging buffer; old USB output also contained a partial line.
  Compact CSV output and draining the backlog resolved those capture problems.

See ../README.md for reproduction commands and ../../../CVB_TIMING_PLAN.md for
commit history and resume status. All timings above use the configured 170 MHz
conversion; external PC7 pulse validation remains available as a follow-up.
