# CVB experiment report: E0 calibration to E1, N = 10

Prepared: 2026-10-06. **Confirmed E1 configuration: N = 10 modules per arm,
20 modules total.** E0 internal measurements are available. E1 firmware is now implemented and compiled; board execution and measurements
are pending. See [E1 execution instructions](../E1_N10.md).

## 1. Experiment definitions

| Experiment | Target | Status |
| --- | --- | --- |
| E0 | Empty target window: measure instrumentation overhead | DWT results collected; oscilloscope E0 pulse still required |
| E0b | Nominal 10 us DWT reference delay | Internal results collected; independent scope clock check pending |
| E1, N = 10 per arm | CVB with 20 modules total | Firmware compiled; board execution and measurements pending |

The completed CVB campaign used **5 modules per arm, 10 modules total**.
Its timing-v2 CSV field `N` is 5. Those results describe two five-element sorts
and cannot be relabelled as the requested ten-element-per-arm experiment.

The user confirmed **10 modules per arm, 20 total** for E1. New firmware
adaptation, correctness validation and measurements are required; no timing
results for that size exist in the collected campaign.

Here E1 is a proposed label for the next CVB experiment. The original timing-v2
experiment numbering uses E1 for its bubble-sort baseline; do not confuse the
experiment label with firmware `TIMING_TEST_MODE`. The current CVB mode is 3.

## 2. Measurement setup

| Item | Recorded configuration |
| --- | --- |
| CVB source | `hackathon_lille/main`, commit `ecb51f5` |
| Processor | SPIN STM32G474, configured at 170 MHz |
| Compiler / OS | GCC ARM 12.3.1 / Zephyr 4.0.0 |
| Optimization | Size optimization, `-Os` |
| Control period | 200 us, TIM6 |
| Timing | DWT cycle counter and GPIO pulse |
| Oscilloscope connection | CH1 on timing output PC7 / SPIN pin 9 and logic GND |
| Power stage / PWM / ADC | Not initialized by the benchmark |

The CVB target includes index reset, voltage/index sorting, and insertion
selection. Input generation and restoration, output consumption and reporting
are outside the target window.

## 3. E0: recorded empty-window results

Source: [E0 raw log](E0.log) and [E0 summary](processed/E0_summary.csv).
Collection: 20 batches, **20,000 executions**.

| Target statistic | CPU cycles | Time (us) |
| --- | ---: | ---: |
| Minimum | 6 | 0.035294 |
| Average | 8 | 0.047059 |
| Observed maximum | 17 | 0.100000 |

| Scheduling diagnostic | Recorded value |
| --- | ---: |
| Body maximum | 0.158824 us |
| Entry-latency maximum | 3.400000 us |
| Average callback period | 200.000000 us |
| Period jitter, max minus min | 2.788235 us |
| Body overruns | 0 |

These E0 results describe the recorded build. After changing workload size or
instrumentation, repeat E0 with the new campaign's build settings.

### Remaining E0 scope calibration

The supplied oscilloscope photograph shows a pulse near 9 us. It is consistent
with the active combined-arm CVB target and **is not an E0 empty-window capture**.

Record the E0 positive pulse width, P0, on the scope before calculating the
external timing offset:

```text
Scope offset = P0 - T0
Corrected CVB scope duration = measured CVB pulse width - scope offset
```

T0 is the corresponding DWT empty-window duration. Use matched measurements
and consistent thresholds/settings; do not subtract unrelated maxima or infer
P0 from the active CVB photograph. E0b scope validation should also be completed.

## 4. Existing CVB results: 10 modules total, 5 per arm

These are previous campaign measurements, provided for comparison. Each row
contains 540,000 executions across 180 input cases.

| Target | Minimum (us) | Average (us) | Observed maximum (us) |
| --- | ---: | ---: | ---: |
| Both arms | 8.700 | 8.929 | 9.312 |
| Upper arm | 4.371 | 4.496 | 4.724 |
| Lower arm | 4.453 | 4.560 | 4.759 |

All 119,556 on-board correctness combinations passed. No body overruns were
observed. See the [full campaign report](REPORT.md) for startup transients,
per-pattern results and measurement boundaries.

### Supplied RIGOL MHO984 photograph

Manually transcribed from `1000102582.jpg`, displaying 2026-10-06. These values
were read from the photograph, not obtained through a SCPI connection.

| CH1 +Width statistic | Displayed value |
| --- | ---: |
| Current | 9.0642 us |
| Average | 8.9234 us |
| Maximum | 9.3857 us |
| Minimum | 8.7642 us |
| Standard deviation | 98.857 ns |
| Count | 1,000 |

The scope average differs from the combined DWT average by approximately 0.063%.
This is an uncalibrated comparison of different sample populations. It is not
evidence of a completed N = 10 per-arm experiment or a formal error bound.

## 5. E1 procedure: N = 10 modules per arm

1. Use the confirmed ten modules per arm, twenty total. Retain both-arm CVB as
   the primary target and collect per-arm runs for comparison. Keep the current
   five-module baseline preserved.
2. Generalize CVB arrays, indices, input patterns, insertion counts, output masks,
   case numbering and reference checks for ten elements. Changing only
   `TEST_SORT_N` does not resize the existing CVB implementation.
3. Preserve the original sorting policy for a baseline comparison: N + 1 full
   passes, N - 1 comparisons per pass, followed by insertion selection. For
   N = 10 this gives 99 comparisons per arm, before considering swap costs.
4. Validate current signs, zero current, insertion counts 0..10, ties and voltage
   orderings. Select a bounded test set suitable for N = 10; do not blindly scale
   the existing exhaustive permutation tests to ten elements.
5. Repeat E0 and E0b with fixed compiler, clock, GPIO method and control period.
   Obtain the scope E0 offset and independent reference-delay measurement.
6. Restore fresh voltages before every measured call. Collect separate, labelled
   upper, lower and combined runs with the same documented input families.
7. Save raw DWT logs, scope statistics, sample counts and build metadata. Report
   raw and offset-corrected scope values separately, with observed maxima.

### E1 results to complete after measurement

| Field | Value |
| --- | --- |
| N definition | Confirmed: number of modules per arm |
| Modules per arm / total | 10 / 20 |
| Firmware commit and image hash | Pending |
| Input pattern, currents and insertion counts | Pending |
| Number of executions | Pending |
| DWT minimum / average / maximum | Not measured for N = 10 per arm |
| Scope current / average / minimum / maximum | Pending |
| Scope standard deviation / count | Pending |
| E0 scope offset | Pending |
| Corrected scope duration | Pending |
| Body overruns and period jitter | Pending |

## 6. Interpretation limits

The callback-body timing excludes subsequent statistics/publication and ISR
exit. A zero body-overrun count does not establish a full production-loop
deadline. Observed maxima are not formal WCET bounds. The initial report was documentation-only. The E1 follow-up adapts and compiles
the source for N=10; it has not flashed the board. Five host analysis tests passed.
