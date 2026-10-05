# CVB timing plan and resume record

Updated: 2026-10-05 (Europe/Paris). Maintain this file at every milestone.

## Objective and authorization

Measure the centralized CVB from `hackathon_lille/main`, original commit
`ecb51f5c807ef0e06e5c59e451165c63c6d227b5`, in this Core project.
The user authorized implementation, local commits, compilation, flashing and
running tests on the connected SPIN board. No remote push is requested.
Use USB-only firmware: never initialize PWM/power outputs or ADC acquisition.

## Current stage

| Stage | Status | Evidence / commit | Next action |
| --- | --- | --- | --- |
| Inspect branch and timing-v2 | Complete | Baseline ecb51f5 preserved | None |
| 1. Isolate existing CVB | Complete | e323140; functions unchanged, build passed | None |
| 2. Integrate timing-v2 | Complete | e2ae546; DWT/PC7/TIM6, 200 us | None |
| 3. Inputs and correctness | Complete | 7f8ef47; 119,556 on-board checks passed | None |
| 4. Documentation and analysis | Complete | ae35b49; four analysis tests passed | None |
| 5. Board experiments | Complete (internal timing) | E0/E0b + three 540,000-sample campaigns; final combined firmware verified | Optional external oscilloscope validation |
| 6. Commit measured results | Complete | 55c309c; report, raw logs, metadata and summaries committed | None |


## Implementation plan

1. Preserve original firmware outside the compiled source directory. Simplify
   `main.cpp`; retain existing upper/lower CVB functions and data types unchanged.
   Remove application RS485, sensors, synchronization, modulation, power control,
   board-role detection and ScopeMimicry from the benchmark execution path.
2. Reuse timing-v2 DWT counting, GPIO pin 9 / PC7 pulses, TIM6 entry latency,
   callback-body duration, period/jitter and background CSV reporting. Use
   `source_tim6`, `task.startCritical(false)`, 200 us period and 1000 samples/batch.
   Keep E0 empty calibration, E0b 10 us reference delay and labelled CVB modes.
3. Primary target: `sorting_upper_arm(); sorting_lower_arm();`. Index resets,
   voltage/index sorting and insertion selection are inside this window. Input
   construction, voltage copies, result consumption and reporting are outside.
   Also collect separate upper/lower-arm measurements. Keep the baseline's six
   passes and four comparisons/pass for five modules/arm; do not optimize it.
4. Restore input arrays before every call. Check ascending, descending, equal,
   partially tied, alternating and reproducible random values; current signs
   positive, negative and zero; insertion counts 0..5. Validate against an
   independent reference, including the existing tie behavior. Prevent compiler
   elimination and inspect optimized target assembly.
5. Build, flash via USB, capture E0/E0b and CVB batches, retain raw logs and
   metadata, and analyze with the timing-v2 CSV reader. Report cycles and us.
   Oscilloscope validation requires actual scope evidence; never claim it from
   the DWT reference delay alone.

The production branch only calls both CVB functions when the **upper-arm**
insertion count changes. Primary measurements force active execution each sample.
These are isolated execution costs, not average costs including skipped calls.
Timing-v2 body timestamps exclude later statistics/publication and ISR exit;
its deadline diagnostic does not certify the complete production loop.

## Commit sequence

| Order | Intended commit | Status / actual hash |
| --- | --- | --- |
| 1 | `refactor: isolate main-branch CVB for benchmarking` | e323140 |
| 2 | `feat: integrate timing-v2 harness for CVB` | e2ae546 |
| 3 | `test: add reproducible CVB cases and correctness checks` | 7f8ef47; 119,556 on-board checks passed |
| 4 | `docs: document CVB timing campaign and analysis` | ae35b49; Python checks passed |
| 5 | `experiment: record CVB timing results` | 55c309c; internal timing results |

Include this file in milestone commits. Record a commit's hash in the next
update (a commit cannot contain its own hash). Do not amend completed commits
just to insert their hashes.

## Environment and resuming

- Workspace: `G:\France\docv\first-work\new_work\Core`.
- Branch at start: `main_lille`.
- Timing source: `G:\France\docv\first-work\Architectural Scaling Laws & Physical Limits\Toulouse\timing_v2\OwnTech_TWIST_Timing_Experiment`.
- PlatformIO: `C:\Users\Dell\.platformio\penv\Scripts\pio.exe`.
- Python: `C:\Users\Dell\.platformio\penv\Scripts\python.exe` (the `py` launcher reports no installed Python).
- Board: USB VID:PID `2FE3:0100`, serial `423250070032003B`, observed COM9.
  Re-enumerate before upload/capture; port may change.
- Baseline build succeeded with GCC ARM 12.3.1, Zephyr 4.0.0, 170 MHz.
- PlatformIO needs access to its cache outside the workspace; sandbox initially
  blocked `C:\Users\Dell\.platformio\platforms.lock`. Elevated build was approved.
- Existing linker warns about an RWX LOAD segment; also present in baseline.
- Shield configuration discrepancy: `platformio.ini` = 1.4.1, `src/app.ini` = 1.4.2.
  Effective baseline CMake shield is `twist_v1_4_1`; benchmark uses no shield outputs.
- Current isolation build output: `.pio/cvb-isolation-build.log`.

On resuming: read this file, run `git status --short --branch` and `git log -6
--oneline`, inspect outstanding changes/logs, then continue the next incomplete
stage. Do not overwrite collected results or re-extract from the simplified main.

## Results and limitations

| Target | Samples | Min us | Average us | Observed max us |
| --- | ---: | ---: | ---: | ---: |
| Both arms | 540,000 | 8.700 | 8.929 | 9.312 |
| Upper arm | 540,000 | 4.371 | 4.496 | 4.724 |
| Lower arm | 540,000 | 4.453 | 4.560 | 4.759 |

No body overruns observed. Full details: [results report](benchmarks/cvb/results/REPORT.md).
Board is running mode 3 / scope 0 (both arms), with no power/ADC initialization.
Final ELF matches the measured combined image and five live batches passed.
No oscilloscope validation performed. Upper-only capture retains a startup period
transient; timing-body diagnostics exclude reporting and ISR exit.

## Resolved issues

- Git identity initially missing; user configured it. First four planned commits exist.
- Zephyr compiler_barrier macro conflicted with timing-v2 helper: renamed helper.
- Verbose reporting overflowed the logging buffer: compact CSV/metadata is default.
- USB backlog contained partial output: capture drains it before accepting records.
- Failed E0 logs are retained in results/diagnostics and excluded from analysis.
- `.vscode/settings.json` has an unrelated editor visibility change; leave it untouched.

## Immediate resume point

All implementation, board runs and five planned commits are complete. The result
commit is `55c309c`. This final documentation update records its hash and completion
state. No required implementation work remains. Do not rerun or overwrite accepted
logs. Optional follow-up: measure PC7 pulses with an oscilloscope for independent
clock and timing validation. The board is currently running both-arm CVB with the
power stage and ADC disabled.
