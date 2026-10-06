> **Current source: E1, 10 modules per arm (20 total).** See [E1 execution guide](E1_N10.md)
> and [version package](versions/E1_N10.zip). The N=5 campaign described below is
> preserved historical data. For E1 capture, pass `--modules-per-arm 10`.

# Centralized CVB timing campaign

This is the lead-board CVB from `hackathon_lille/main` at `ecb51f5`, with
five modules per arm. The original application is preserved in
`reference/main_lille_ecb51f5.cpp`. Both sorting function bodies in
`src/cvb_algorithm.h` are unchanged. The functions reset local indices, sort
floating-point voltages and indices using six full bubble-sort passes, then
select inserted modules. Equal voltages retain ascending local index order;
negative current traverses that order backwards.

## Firmware layout and measurement boundaries

- `src/main.cpp`: run correctness checks, then start the timing harness.
- `src/cvb_algorithm.h`: original CVB functions and required variables.
- `src/cvb_cases.h`: inputs, independent correctness reference, output consumer.
- `src/timing_config.h`: compile-time mode and arm selection.
- `src/timing_harness.h`: adapted timing-v2 DWT/GPIO/TIM6 infrastructure.

The measured target calls both CVB functions (scope 0), upper only (scope 1), or
lower only (scope 2). Index resets and insertion selection are included. Preparing
and restoring voltage arrays, consuming outputs, statistics and reporting are
outside the target. Wrapper call overhead remains included. Each callback runs
an active CVB step; the production conditional on upper insertion-count changes
is deliberately outside this benchmark.

No power-stage, PWM, ADC, RS485, synchronization or encoder initialization is
performed. Connect the SPIN board by USB. STLINK is unnecessary. For independent
validation, connect oscilloscope CH1 to SPIN header pin 9 / PC7 and logic GND.
The timing pin toggles during the measured target; do not enable the TIM3 encoder.

The period is 200 us using TIM6 and `task.startCritical(false)`. DWT conversion
uses the configured 170 MHz clock. The effective build uses spin 1.2.0 and TWIST
1.4.1 from `platformio.ini`; the old `src/app.ini` example says 1.4.2 but is not
included by the current PlatformIO configuration. No shield output is used.
Optimization remains Zephyr size optimization (`-Os`) across all runs.

## Correctness and input campaign

Before starting any timing mode, the board checks **119,556 combinations**:
six campaign patterns, all 243 arrangements of three repeated voltage levels,
and all 120 permutations of five distinct voltages; each is checked for all nine
upper/lower current-sign pairs and all 36 upper/lower insertion-count pairs.
An independent rank reference checks gate outputs, ordering, stable ties and
index/voltage consistency. Failure prevents the critical task from starting.

Each CVB image continuously repeats 180 cases:

- Six patterns: ascending, descending, all equal, partially tied, alternating,
  and fixed-seed pseudorandom.
- Five upper/lower current pairs: (+1,+1), (-1,-1), (+1,-1), (-1,+1), (0,0).
- Upper insertion counts 0..5, with complementary lower counts 5..0.

Each case runs for three batches of 1000 samples: 540,000 executions per full
scope campaign, nominally 108 seconds. Both arms use the same voltage pattern;
a small runtime offset changes without changing ordering/ties. Voltage arrays
are restored every callback because the CVB functions sort them in place.

The capture tool requires all 540 batch phases, even if connection starts in the
middle of the repeating campaign. It verifies configuration and self-test
metadata, detects dropped reports, and refuses to overwrite existing logs.
It first drains the old USB CDC backlog, which can contain truncated records
when the host was not reading. Compact output avoids overflowing Zephyr's
1024-byte logging buffer; verbose reporting is disabled by default.

## Build, upload and capture (PowerShell, project root)

```powershell
$pio = 'C:\Users\Dell\.platformio\penv\Scripts\pio.exe'
$python = 'C:\Users\Dell\.platformio\penv\Scripts\python.exe'

# E0: empty instrumentation window
& $python benchmarks/cvb/select_run.py 0
& $pio run -e USB -t upload
& $python benchmarks/cvb/record_build.py E0 --date 2026-10-05
& $python benchmarks/cvb/capture.py benchmarks/cvb/results/E0.log --mode 0

# E0b: DWT reference delay; scope needed for independent clock validation
& $python benchmarks/cvb/select_run.py 2
& $pio run -e USB -t upload
& $python benchmarks/cvb/record_build.py E0b --date 2026-10-05
& $python benchmarks/cvb/capture.py benchmarks/cvb/results/E0b.log --mode 2

# Both-arm CVB; repeat with --scope 1 and 2 and distinct output filenames
& $python benchmarks/cvb/select_run.py 3 --scope 0
& $pio run -e USB -t upload
& $python benchmarks/cvb/record_build.py CVB_both --date 2026-10-05
& $python benchmarks/cvb/capture.py benchmarks/cvb/results/CVB_both.log --mode 3 --scope 0
& $python benchmarks/cvb/analyze_campaign.py benchmarks/cvb/results/CVB_both.log --output-dir benchmarks/cvb/results/processed
```

Close other serial monitors before capture/upload. The default board serial is
`423250070032003B`; override `--serial-number` in capture for a different board.
PlatformIO re-enumerates the board for USB bootloader upload. Use new filenames
for repeated campaigns; retain the associated build hashes/configuration.
After per-arm runs restore mode 3/scope 0, rebuild and flash that default image.

Calibration summaries use the upstream reader, copied unchanged with its MIT
license in `reference/TIMING_LICENSE`:

```powershell
& $python benchmarks/cvb/timing_analysis.py benchmarks/cvb/results/E0.log --output benchmarks/cvb/results/processed/E0_summary.csv
& $python -m unittest discover -s benchmarks/cvb/tests -v
```

`CVB_META` records contain: batch, case, scope, pattern, upper/lower current,
upper/lower insertion count, self-test check count, failure count, output mask.
The following `CSV` record keeps the timing-v2 format unchanged. Analysis groups
three distinct batch phases per case and emits per-case CSV plus overall JSON.
Batch averages are integer cycles, so aggregate averages inherit truncation.

## Interpreting results

Report raw target min/average/max cycles and microseconds. E0 describes the
instrumentation floor; do not silently subtract its maximum from CVB maxima.
For an oscilloscope, measure E0 pulse width P0 and use offset `P0 - T0`; compare
the corrected CVB pulse to DWT. Short windows need absolute-error reporting.
E0b measured only by DWT is not independent proof of the CPU clock.

The reported body duration excludes statistics/publication after its final
timestamp and ISR return. The overrun counter checks body >= period, while the
deadline diagnostic additionally checks the sum of separately observed maximum
entry latency and maximum body time. This is a benchmark diagnostic, not a
complete ISR duration, a production-loop guarantee, or a formal WCET bound.
Input restoration warms the working data and the results describe that setup.

For current progress and commit hashes, see the root `CVB_TIMING_PLAN.md`.

## Read-only oscilloscope LAN queries

`rigol_lan_readonly.py` uses Python's standard-library TCP socket; VISA and driver
installation are not required. Supply the scope's real IPv4 address:

```powershell
& 'C:\Users\Dell\.platformio\penv\Scripts\python.exe' benchmarks/cvb/rigol_lan_readonly.py YOUR_SCOPE_IP
```

Replace `YOUR_SCOPE_IP` with the numeric address displayed on the scope.
The default port 5555 is a candidate, **not yet verified on this MHO984**; use
`--port` for the port in its SOCKET resource address. A successful TCP connection
tests that endpoint. The script checks `*IDN?` before measurement queries and
reads the instrument's reported LXI/SOCKET addresses, CH1 positive width and
existing current/average/minimum/maximum/deviation/count statistics. Commands
follow the [MHO900 programming guide](https://www.rigol.com/dam/global/downloads/brochures/en/program-guide/oscilloscopes/MHO900-ProgrammingGuide.pdf).

Only allowlisted queries are sent. No reset, run/stop, acquisition, trigger,
channel, statistics-enable or statistics-reset command is sent. The SPIN COM
port and firmware are not accessed. Missing/disabled measurements are retained
as unavailable rather than enabled; a transport timeout stops the session to
avoid misattributing delayed responses. Sequential readings are not simultaneous.

JSON results preserve the reply text and exact response bytes as hex, plus
microsecond conversions for valid width values. Output files are never
overwritten. A real instrument connection remains unverified until the actual
IP is supplied and a successful identification is captured; no working LAN
method is claimed yet.
