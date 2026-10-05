# OwnTech SPIN/TWIST Timing Benchmark — Experiment Setup Guide

> GitHub text export of the supplied English guide. The original source was a DOCX document.

<PARSED TEXT FOR PAGE: 1 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 1 of 21
OwnTech SPIN / TWIST Timing Benchmark
Experiment Setup Guide v2 — review of  wiring, build settings, procedure E0–E7, data 
recording, complete source code
Companion to: OwnTech_TWIST_Timing_Program_EN.docx and main_timing_EN.cpp . This guide 
supersedes the  code listing with main.cpp.
Safety. Experiments E0–E6 run with the TWIST power stage unpowered: USB only. No PWM, no power-stage 
enable and no ADC acquisition is started by the benchmark. E7 (full control loop with PWM) is a separate 
experiment and follows the OwnTech power-up procedure of your lab.
1. Review of version 1 — verdict and changes..........................................................................................................1
2. Equipment..............................................................................................................................................................1
3. Pin selection and wiring......................................................................................................................................... 1
3.1 SPIN header pin → STM32 pin map.................................................................................................................1
3.2 Oscilloscope wiring.......................................................................................................................................... 1
4. Software settings....................................................................................................................................................1
4.1 Project files...................................................................................................................................................... 1
4.2 Compile-time switches in main.cpp.................................................................................................................1
4.3 Console.............................................................................................................................................................1
5. What the program measures................................................................................................................................. 1
5.1 Equations......................................................................................................................................................... 1
6. Oscilloscope settings.............................................................................................................................................. 1
7. Procedure...............................................................................................................................................................1
7.1 Preparation (once)...........................................................................................................................................1
7.2 E0 — instrumentation calibration (mode 0).................................................................................................... 1
7.3 E0b — clock validation (mode 2)..................................................................................................................... 1
7.4 E1–E5 — bubble-sort sweep (mode 1)............................................................................................................1
7.5 E6 — MMC algorithm in the target window....................................................................................................1
7.6 E7 — full control loop...................................................................................................................................... 1
8. Data recording....................................................................................................................................................... 1
8.1 CSV line format................................................................................................................................................ 1
8.2 DWT results (fill in)...........................................................................................................................................1
8.3 Oscilloscope cross-validation (fill in)................................................................................................................ 1
8.4 Configuration control (fill in once per campaign)............................................................................................ 1
8.5 Acceptance criteria..........................................................................................................................................1
9. Troubleshooting..................................................................................................................................................... 1
10. Complete source code — main.cpp (v2).............................................................................................................. 1
11. References...........................................................................................................................................................1<PARSED TEXT FOR PAGE: 2 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 2 of 21
1. Review
Item Verified against (OwnTech Core / ARM)
spin.gpio.configurePin/setPin/resetPin
with pin 9
GpioHAL.h/.cpp: pin is uint8_t; 9 → GPIOC bit 7 (PC7); same 
pin as OwnTech's own GPIO example; power-leg.yaml table 
confirms PC7 = SPIN 9
task.createCritical(fn, 100, source_tim6) TaskAPI.h: period 1…6553 µs, returns 0/-1; 
uninterruptible_synchronous_task.cpp: TIM6 path
task.startCritical(false) scheduling_start_uninterruptible_synchronous_task(): 
with TIM6 and the default argument (true) it divides by the 
HRTIM period, which is 0 when PWM is not initialised, and 
returns WITHOUT starting the timer
Execution context of the critical task stm32_timer_driver.c: TIM6 UPDATE IRQ registered with 
IRQ_ZERO_LATENCY → the task is an ISR above the kernel's 
priority mask
DWT_CYCCNT register map ARM DDI0439 / ST PM0214
CPU_FREQ_HZ = 170 MHz hard-coded spin.dts: rcc clock-frequency = 170 MHz; the TIM6 driver 
computes its prescaler from 
CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
GPIO edges through the Zephyr GPIO API
Works; ~100–200 cycles per edge, 
asymmetric around the DWT window
GpioHAL.cpp → if/else pin lookup → gpio_pin_set() (with 
CONFIG_ASSERT=y)
Interrupt-entry latency TIM6 counts 0.1 µs ticks from 0 at each event
Actual period / jitter / overruns —
Machine-readable output —
g_snapshot.batch_id++ on a volatile GCC
sum_cycles as uint32
Fine at Ts = 100 µs (overflow only beyond 
25 ms per sample)
—
Unused static functions in mode 0 —
Pin conflict risk hrtim.dtsi: PC7 is HRTIM TIMF2, muxed only when PWMF is 
initialised; spin.dts: TIM3 encoder pinctrl (PC6/PC7) applied 
only if the encoder is started
What did not change: the experimental logic (E0 calibration → bubble sort sweep → MMC), the batch/snapshot 
architecture, the two-method cross-validation and the acceptance criteria of the v1 document remain valid. Use 
the v1 document for the method description and this guide for execution.
2. Equipment
Item Requirement Why
OwnTech SPIN 1.2.0 Programmed through USB-C; TWIST 1.4.2 shield may stay 
mounted
Target MCU STM32G474RE at 170 
MHz
USB-C cable to PC Data-capable cable Power, firmware upload, CDC-ACM 
console (115200)
Oscilloscope ≥ 50 MHz analogue bandwidth; pulse-width (+Width) 
measurement with Min/Mean/Max statistics; infinite 
persistence useful
External measurement of the target 
duration (method B)
Probe 10× passive probe, short ground spring/lead 3.3 V logic edges; keep ground path 
short to avoid ringing
Connection to pin 9 2.54 mm header pin or probe hook on SPIN header pin 9, 
plus a GND pin
Benchmark pulse output (PC7)<PARSED TEXT FOR PAGE: 3 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 3 of 21
Item Requirement Why
PC software VS Code + PlatformIO + an OwnTech Core project that 
already builds and uploads an official example
Toolchain (Zephyr, arm-none-eabi￾gcc) comes with the OwnTech 
project
Optional: ST-LINK Only for debugging; not needed for the benchmark DWT works without a debugger 
attached (TRCENA is set by software)
3. Pin selection and wiring
3.1 SPIN header pin → STM32 pin map
The table is a transcription of GpioHAL::getPinNumber() and GpioHAL::getGpioDevice() (OwnTech Core). Only 
the 45 GPIO-capable header pins appear; the remaining header positions (3, 8, 13, 18, 23, 28, 33, 36, 38–40, 54, 
57) are not GPIO in the API (power, ground or reserved — check the SPIN 1.2.0 pinout figure for the GND 
positions). The “use” column lists what the SPIN device tree or the TWIST 1.4.2 overlay assigns to the pin; “—” 
means neither file references it.
SPIN pin STM32 Use in spin.dts / TWIST 1.4.2 overlay / hrtim.dtsi Probe candidate
1 PB11 — yes
2 PB12 TWIST LEG2 PWM high (HRTIM PWMC1) no
4 PB13 TWIST LEG2 PWM low (HRTIM PWMC2) no
5 PB14 HRTIM PWMD1 (unused by TWIST) possible
6 PB15 HRTIM PWMD2 (unused by TWIST) possible
7 PC6 TWIST LEG1 capacitor pin; HRTIM PWMF1; TIM3 CH1 (encoder) no
9 PC7 HRTIM PWMF2 (unused by TWIST); TIM3 CH2 (only if encoder started). 
OwnTech GPIO example pin.
RECOMMENDED
10 PC8 HRTIM PWME1 (unused by TWIST) alternate
11 PC9 HRTIM PWME2 (unused by TWIST) alternate
12 PA8 TWIST LEG1 PWM high (HRTIM PWMA1) no
14 PA9 TWIST LEG1 PWM low (HRTIM PWMA2) no
15 PA10 HRTIM PWMB1 (unused by TWIST) possible
16 PC10 USART3 TX (RS485 on TWIST) no
17 PC11 USART3 RX (RS485 on TWIST) no
19 PC12 TWIST LEG1 driver enable no
20 PB4 — yes
21 PB9 — yes
22 PC13 TWIST LEG2 driver enable no
24 PC0 TWIST sensor input (ADC) no
25 PC1 TWIST LEG2 current-mode / sensor input (ADC) no
26 PC2 — (ADC-capable) yes
27 PC3 TWIST sensor input (ADC) no
29 PA0 TWIST sensor input (ADC) no
30 PA1 TWIST LEG1 current-mode / sensor input (ADC) no
31 PB0 — yes
32 PA5 SPIN user LED no
34 PA6 — yes
35 PC4 — yes
37 PB1 HRTIM SCOUT — TWIST sync output no
41 PB10 — yes<PARSED TEXT FOR PAGE: 4 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 4 of 21
SPIN pin STM32 Use in spin.dts / TWIST 1.4.2 overlay / hrtim.dtsi Probe candidate
42 PB2 HRTIM SCIN — TWIST sync input no
43 PC5 — yes
44 PA7 — yes
45 PA4 — yes
46 PA13 SWDIO (debug) NEVER
47 PA14 SWCLK (debug) NEVER
48 PA15 — yes
49 PD2 TIM3 ETR (encoder, only if started) possible
50 PB3 TIM4 ETR (encoder, only if started) possible
51 PA2 LPUART1 TX (SPIN) no
52 PA3 LPUART1 RX (SPIN) no
53 PB5 FDCAN2 RX (TWIST) no
55 PB6 FDCAN2 TX (TWIST); TIM4 CH1 no
56 PB7 TWIST LEG2 capacitor pin; TIM4 CH2 no
58 PB8 — yes
Why pin 9 is safe. PC7 is the second output of HRTIM unit F. OwnTech muxes HRTIM pins to the timer only 
inside hrtim_tu_gpio_init(), i.e. when spin.pwm.initUnit(PWMF) (or the shield power API for a leg that uses 
PWMF) is called. TWIST legs use PWMA (LEG1) and PWMC (LEG2), so PC7 remains a plain GPIO even in E7. The 
TIM3 incremental-encoder pinctrl named incremental_encoder on PC6/PC7 is applied only if you start the 
encoder — do not use the TIM3 encoder API while benchmarking.
If you change TIMING_GPIO_PIN, the code resolves the port and bit for you (also for STM32-style names such as
PC8) and rejects non-GPIO pins and the SWD pins at compile time.
3.2 Oscilloscope wiring
 SPIN 1.2.0 (+ TWIST 1.4.2, power stage OFF)
 PC <==== USB-C =====> [USB-C] power + firmware upload + CDC-ACM console 115200
 |
 Oscilloscope CH1 | header pin 9 (STM32 PC7) o----> probe TIP
 10x probe, DC, 1 Mohm | header GND pin o----> probe GROUND clip
 |
 Trigger: CH1 rising edge ~1.6 V | Nothing else connected. No DC bus. No load.
• Probe ground goes to a SPIN GND header pin (logic ground, the same ground as the USB connector). 
Never to a power-stage node.
• Keep the ground lead as short as possible (ground spring if available); the pulse edges are ~1–2 ns on a 3.3
V output and will ring with a long lead, which perturbs the +Width measurement.
• Use 10× attenuation and DC coupling; set the channel to 1 V/div so 3.3 V fills about a third of the screen.
• The pulse repeats every Ts = 100 µs (10.00 kHz). If you see a different repetition rate, the configured 
period or the trigger source is not what you think — stop and check.
• For E7 (power stage energised) follow the lab's isolation rules; the SPIN logic ground reference of the 
TWIST may not be at earth potential depending on the converter configuration. Use a differential or 
isolated probe if in doubt.
4. Software settings
4.1 Project files<PARSED TEXT FOR PAGE: 5 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 5 of 21
File Setting Value / action
src/main.cpp Program Replace with main.cpp v2 (Section 10). Back up the previous file first.
platformio.ini board / board_version spin / 1_2_0 (default in the Core project; set to your SPIN revision)
platformio.ini board_shield / 
board_shield_version
twist / 1_4_2 — keep for E6/E7. For E0–E5 you may comment both 
lines out to build a bare SPIN image; then record that choice, because it
changes which OwnTech modules are compiled.
platformio.ini monitor_speed 115200 (the console is USB CDC-ACM; the number must match the 
Serial Monitor setting)
platformio.ini build_flags -std=c++2a -fsingle-precision-constant (OwnTech default; do not 
add -O flags here — optimisation is a Zephyr Kconfig choice, see 4.2)
zephyr/prj.conf Optimisation level Default is size optimisation (-Os). For control code you may add 
CONFIG_SPEED_OPTIMIZATIONS=y (-O2). Whatever you choose, 
keep it identical for E0…E7 and record it.
zephyr/prj.conf Nothing else required CONFIG_PRINTK, CONFIG_FPU, zero-latency IRQs and asynchronous 
tasks are already enabled by the OwnTech defaults.
How to see which optimisation level was used: after a build, open .pio/build/<env>/zephyr/.config and look for
CONFIG_SIZE_OPTIMIZATIONS=y or CONFIG_SPEED_OPTIMIZATIONS=y. Write the value in the configuration￾control row of the result tables.
4.2 Compile-time switches in main.cpp
Macro Default Meaning / when to change
CONTROL_PERIOD_US 100 Ts. 1…6553 µs with TIM6 (checked at compile time). Increase to 200 for E5 if N = 
64 overruns.
TIMING_IRQ_SOURCE_TIM6 1 1 = TIM6 trigger (E0–E6). 0 = HRTIM master trigger for E7; then PWM must be 
initialised before createCritical() and Ts must be a multiple of the PWM period.
TIMING_GPIO_PIN 9 Oscilloscope pin: SPIN number (1…58) or STM32 name (PC8…). Port/bit resolved 
and checked at compile time.
BATCH_SAMPLES 1000 Executions per reported batch (0.1 s at Ts = 100 µs). 10000 gives one report per 
second with a better per-batch maximum.
MAX_SORT_N / TEST_SORT_N 64 / 16 Bubble-sort size. Sweep TEST_SORT_N = 4, 8, 16, 32, 64 (E1–E5).
REFERENCE_DELAY_US 10 Length of the DWT-timed busy-wait in mode 2 (clock validation).
TIMING_TEST_MODE 1 0 = empty window (E0), 1 = bubble sort (E1–E5), 2 = reference delay (E0b).
TIMING_USE_STM32_LL 1 1 = direct GPIO register writes (≈1 cycle per edge) and TIM6->CNT latency read. 0 
= OwnTech GPIO API only (portable; set 0 if <stm32_ll_gpio.h> is not found by 
your build).
PRINT_CSV 1 Print one CSV line per batch for copy-paste into a spreadsheet.
Rules for the whole campaign: one firmware image per experiment; change only the macro the experiment 
calls for; never change optimisation level, shield selection, Ts or TIMING_USE_STM32_LL between E0 and the 
experiment you compare it with. The batch header line prints Ts, f_CPU, pin and LL on every report so a log 
file is self-describing.
4.3 Console
• The console is the USB virtual COM port of the SPIN (zephyr,console = &cdc_acm_uart0), not the ST-LINK 
UART. Open the PlatformIO Serial Monitor (115200). The OwnTech default CONFIG_BOOT_DELAY=1500
gives 1.5 s to attach; if the boot banner is missed, every batch report repeats the configuration line.
• Let the monitor log to a file (PlatformIO: monitor_filters / log2file, or any terminal with logging) — the CSV
lines are what you paste into the result tables.<PARSED TEXT FOR PAGE: 6 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 6 of 21
• The reporter prints only from the background thread; the critical task never prints (it is a zero-latency ISR 
— a printk there would be a fault or a corrupted console).
5. What the program measures
 TIM6 UPDATE event, t = 0 (TIM6->CNT restarts from 0, ticks of 0.1 us)
 |
 | L = entry latency: NVIC entry + Zephyr ISR wrapper + timer_stm32_callback()
 | + user_task_proxy() -> measured as TIM6->CNT at the first instruction
 v
 loop_critical_task()
 |- read TIM6->CNT = L -+
 |- DWT critical_start |
 |- prepare_sort_input() (mode 1, outside the target) |
 |- GPIO HIGH ---------------------------------+ | C = critical body
 |- DWT target_start | |
 |- task_under_test() = T | P = scope pulse |
 |- DWT target_end | |
 |- GPIO LOW ---------------------------------+ |
 |- result guard (mode 1) |
 |- DWT critical_end -+
 '- statistics + snapshot (not measured, ~50-100 cycles)
 ISR exit -> background thread resumes ... next TIM6 event after Ts (period = entry-to-entry)
Quantity Symbol How Resolution
Target duration T DWT_CYCCNT difference around task_under_test() 1 cycle = 5.882 ns
Critical body C DWT_CYCCNT from first to last instruction of the callback 
(excludes statistics)
1 cycle
Entry latency L TIM6->CNT at callback entry × 17 cycles per tick (TIM6 source 
only)
0.1 µs = 17 cycles
Period — DWT critical_start[n] − critical_start[n−1]; jitter = max − min 1 cycle
Overruns — Count of executions with C ≥ Ts (per batch and since boot) —
Scope pulse P +Width of the HIGH pulse on pin 9; P = T + δ where δ is the E0 
offset
scope-dependent
5.1 Equations
t [us] = cycles / f_CPU[MHz] (170 MHz: 1 cycle = 5.882 ns, Ts = 100 us = 17 000 cycles)
Worst case = L_max + C_max (ISR exit path, ~20-40 cycles, is not included)
Slack = Ts - (L_max + C_max)
Utilisation = (L_max + C_max) / Ts * 100 %
Scope offset : delta = P_E0 - T_E0 (from E0, mode 0)
Cross-check : error % = | (P - delta) - T | / T * 100 (per experiment, using max or mean consistently)
Clock check : f_actual = f_assumed * T_DWT,E0b / (P_E0b - delta) (E0b, mode 2; expect 170.0 MHz +/- crystal tolerance)
Deadline verdict printed by the firmware: PASS when no overrun occurred in the batch and L_max + C_max < 
Ts; otherwise FAIL with the overrun counts. A FAIL is a valid experimental result (e.g. N = 64 at Ts = 100 µs) — 
the firmware keeps running and the scope still shows the pulse.
Terminology: all maxima are observed maxima over the samples collected, not a formal WCET. Say “measured 
maximum over n samples” in the thesis unless a static analysis is added.
6. Oscilloscope settings
Setting Value Comment
CH1 1 V/div, DC, 10× probe 3.3 V logic<PARSED TEXT FOR PAGE: 7 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 7 of 21
Setting Value Comment
Trigger CH1, rising edge, level ≈ 1.6 V, mode Normal One trigger per Ts; Normal mode so nothing is drawn
without a pulse
Timebase E0 100–200 ns/div Pulse ≈ tens of ns with LL = 1; ≈ 1–2 µs with LL = 0
Timebase E0b 2 µs/div Pulse ≈ 10 µs + δ
Timebase E1–E5 start at 5 µs/div, then adapt Pulse grows ≈ ×4 per doubling of N
Measurement +Width on CH1 with statistics (Min / Mean / Max /
count)
Reset statistics at the start of each experiment; 
collect ≥ 10 s (≥ 100 000 pulses)
Acquisition Normal (no averaging), highest sample rate, 
infinite persistence on
Averaging would hide the maximum; persistence 
shows the jitter envelope
Sanity check Frequency / period measurement on CH1 Must read 10.00 kHz / 100.0 µs for Ts = 100 µs
7. Procedure
7.1 Preparation (once)
1. Open the OwnTech project that already builds and uploads an official example. Build it once to be sure the
toolchain works.
2. Back up src/main.cpp; copy main.cpp v2 in its place.
3. Check platformio.ini: board_version matches your SPIN; shield lines as decided; monitor_speed = 115200.
Decide the optimisation level (4.1) and do not touch it again.
4. Wire the probe (3.2). Power the board through USB only.
5. Build with TIMING_TEST_MODE 0 and upload. Open the Serial Monitor. You must see the banner with 
“DWT cycle counter: OK”, the CSV_HEADER line, and then a batch report every 100 ms. If you see “ERROR:
critical task not started”, read Section 9.
7.2 E0 — instrumentation calibration (mode 0)
• Firmware: TIMING_TEST_MODE = 0, TIMING_USE_STM32_LL = 1 (and optionally repeat with 0 to quantify 
the OwnTech GPIO API path).
• Record from the console: Target min/avg/max (expected: a few cycles — the empty noinline call plus two 
DWT reads), Critical body (expected ≈ 10–30 cycles), Entry latency (expected roughly 0.3–1.0 µs; quantised
to 0.1 µs), Period min/avg/max (expected 17 000 ± a few tens of cycles), overruns = 0.
• Record from the scope: P_E0 Min/Mean/Max. Compute δ = P_E0,mean − T_E0,mean. With LL = 1 expect δ
in the tens of nanoseconds; with LL = 0 expect ≈ 1–2 µs.
• If the entry latency is much larger than 1 µs or the period jitter is more than a few hundred cycles with the
board idle, something else is running (USB console traffic is expected to add some jitter; CAN/RS485 or 
encoder activity should not be present).
7.3 E0b — clock validation (mode 2)
• Firmware: TIMING_TEST_MODE = 2, REFERENCE_DELAY_US = 10.
• DWT will report T ≈ 1700 cycles (+ loop exit, a few cycles) by construction. The oscilloscope is the 
independent reference: expect P_E0b = 10.00 µs + δ.
• Compute f_actual with the equation in 5.1. If P − δ = 10.6 µs the core runs at 160 MHz, not 170 MHz, and 
every DWT-derived time in the campaign must be rescaled. A result within ±0.1 % validates CPU_FREQ_HZ
(the HSE crystal tolerance dominates).
7.4 E1–E5 — bubble-sort sweep (mode 1)
• Firmware: TIMING_TEST_MODE = 1; one build per N = 4, 8, 16, 32, 64 (TEST_SORT_N). Everything else 
fixed.<PARSED TEXT FOR PAGE: 8 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 8 of 21
• For each N: wait for at least 10 batches, copy 3–5 CSV lines, take the largest t_max and c_max among 
them; reset the scope statistics, collect ≥ 10 s, record P Min/Mean/Max.
• Expected shape: T grows ≈ N(N−1)/2 × (cycles per compare-and-swap). Doubling N should multiply T by ≈ 
4. At -Os, N = 16 is of the order of 1–2 k cycles (6–12 µs) and N = 64 of the order of 20–30 k cycles (120–
180 µs), i.e. N = 64 is expected to overrun Ts = 100 µs. If it does, that is the result for E5 at 100 µs; add an 
E5 with CONTROL_PERIOD_US = 200 and note it. ′
• Fill the DWT table (8.2) and the cross-validation table (8.3). The error between (P − δ) and T should be well
below 1 % for N ≥ 16; for E1 (N = 4, pulse of a few hundred ns) expect a larger relative error — the scope 
resolution dominates there.
7.5 E6 — MMC algorithm in the target window
• Replace the body of task_under_test() by a call to the real function (e.g. the local sorting / capacitor￾voltage balancing step). Keep input preparation outside the window, as prepare_sort_input() does; keep a
visible side effect after the window (like g_result_guard) so the optimiser cannot delete the computation.
• Replace the worst-case input generator by inputs that are hard for your algorithm (reverse-sorted 
voltages, all-equal voltages, alternating patterns) and run each pattern as a separate sub-experiment; 
report the largest maximum.
• Keep TIM6 as source and startCritical(false): no ADC, no PWM. Pin 9 stays valid.
7.6 E7 — full control loop
• Set TIMING_IRQ_SOURCE_TIM6 0 if the task must be synchronised with the PWM; initialise the PWM 
units (and ADC) before task.createCritical(); Ts must be an integer multiple of the PWM period; call 
task.startCritical(true) if the OwnTech data dispatch must feed the ADC values.
• Entry latency is then reported as n/a (TIM6 is not the trigger). T, C, period and overruns remain valid. 
Widen the DWT/GPIO window to the whole control path (acquisition read → control → sorting → 
communication → PWM update).
• Pin 9 (PC7 = HRTIM PWMF2) remains free as long as PWMF is not initialised — TWIST uses PWMA and 
PWMC. If your E7 code uses PWMF, move the timing pin (e.g. SPIN 26 = PC2, 31 = PB0).
8. Data recording
8.1 CSV line format
One line per batch: 
CSV,batch,mode,N,samples,t_min,t_avg,t_max,c_min,c_avg,c_max,l_min,l_avg,l_max,p_min,p_avg,p_max,ove
rruns,budget_cycles. All durations are in CPU cycles; divide by 170 to get µs. The header line CSV_HEADER,... is 
printed once at boot.
Column Meaning
batch Batch counter since boot
mode, N TIMING_TEST_MODE and TEST_SORT_N of the firmware
samples Executions in the batch (= BATCH_SAMPLES)
t_min, t_avg, t_max Target T (task_under_test only), cycles
c_min, c_avg, c_max Critical body C, cycles
l_min, l_avg, l_max Entry latency L, cycles (0 when not measured)
p_min, p_avg, p_max Entry-to-entry period, cycles (nominal 17 000)
overruns Executions with C ≥ Ts in this batch
budget_cycles Ts in cycles (17 000 at 100 µs / 170 MHz)
8.2 DWT results (fill in)<PARSED TEXT FOR PAGE: 9 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 9 of 21
Exp. Mode / N Samples T max [cyc] C max [cyc] L max [cyc] Period jitter 
[cyc]
Overruns L+C max 
[µs]
Slack 
[µs]
PASS/FAIL
E0 0 / – 1000
E0b 2 / 10 µs 1000
E1 1 / 4 1000
E2 1 / 8 1000
E3 1 / 16 1000
E4 1 / 32 1000
E5 1 / 64 1000
E6 MMC
E7 Full loop
8.3 Oscilloscope cross-validation (fill in)
Exp. N T_DWT mean
[µs]
T_DWT max 
[µs]
P mean [µs] P max [µs] δ (E0) [µs] P−δ mean 
[µs]
Error % 
(mean)
Notes
E0 – — — — δ = P_E0 − T_E0
E0b 10 µs f_actual = …
E1 4
E2 8
E3 16
E4 32
E5 64
8.4 Configuration control (fill in once per campaign)
Item Value
Date / operator
SPIN revision / TWIST revision (or “no shield”)
OwnTech Core commit or release
Optimisation (CONFIG_SIZE_OPTIMIZATIONS or 
CONFIG_SPEED_OPTIMIZATIONS)
Ts (CONTROL_PERIOD_US), IRQ source
TIMING_USE_STM32_LL, TIMING_GPIO_PIN
f_CPU printed at boot / f_actual from E0b
Oscilloscope model, probe, bandwidth
8.5 Acceptance criteria
• Deadline: no overrun and L_max + C_max < Ts in every reported batch of the experiment.
• Slack: positive with the design margin you set (e.g. ≥ 30 % of Ts for the MMC task).
• Repeatability: T_max does not drift between batches by more than the entry-latency quantum plus a few 
cycles once the pattern is fixed.<PARSED TEXT FOR PAGE: 10 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 10 of 21
• Cross-validation: |(P − δ) − T| / T ≤ 1 % for T ≥ 5 µs; for shorter pulses report the absolute difference 
instead.
• Clock: f_actual from E0b within ±0.1 % of 170 MHz; otherwise rescale all DWT values and investigate the 
clock tree.
• Configuration control: table 8.4 completely filled; one firmware image per experiment.
9. Troubleshooting
Symptom Likely cause What to do
No console output at all Wrong COM port (SPIN enumerates as a USB 
CDC device, not the ST-LINK port); monitor 
speed ≠ 115200; cable without data lines
Select the port named “SPIN”/OwnTech; set 
115200; try another cable.
Banner missing, batches 
present
Serial Monitor attached after the 1.5 s boot 
delay
Normal — the batch header repeats Ts, 
f_CPU, pin and LL.
“ERROR: critical task not started
(create=-1, dwt=1)”
Period outside 1…6553 µs, or a critical task 
already defined and running (second 
createCritical)
Check CONTROL_PERIOD_US; make sure no 
other code created a critical task.
“ERROR: critical task not started
(create=0, dwt=0)”
DWT not counting Should not happen on STM32G474. Power￾cycle; check that no debugger script disabled 
TRCENA.
Banner OK, no batch reports Task defined but not running: 
startCritical(true) used with TIM6 and no 
PWM (returns silently); or background task 
not created
Keep task.startCritical(false) for E0–E6; check 
the “could not create background task” 
message.
No pulse on the scope, console 
fine
Probe on the wrong header pin; PC7 claimed 
by PWMF or by the TIM3 encoder; probe 
ground open
Verify the “Timing pin: SPIN 9 -> PC7” line; 
confirm no PWMF/encoder init; check ground 
clip.
Pulse much longer than T_DWT 
(≈ 1–2 µs offset)
TIMING_USE_STM32_LL = 0: Zephyr GPIO API 
latency with CONFIG_ASSERT=y
Expected for the API path; subtract δ from E0, 
or use LL = 1.
Pulse rate ≠ 10 kHz Different Ts in the firmware; HRTIM source 
with another repetition
Compare with the Ts printed in the batch 
header.
Period jitter of several µs, 
latency 1 µs ≫
Another zero-latency or high-priority interrupt
is active (RS485 RX, CAN, ADC DMA)
Disable communication modules for the 
benchmark; record what is enabled.
Build error: stm32_ll_gpio.h / 
stm32_ll_tim.h not found
LL include path not exposed in your Core 
version
Set TIMING_USE_STM32_LL = 0 (portable 
path; entry latency becomes n/a).
Build error: static_assert “not a 
GPIO-capable SPIN pin”
TIMING_GPIO_PIN is a power/GND/reserved 
position or a typo
Use a pin from table 3.1.
Deadline FAIL with overruns > 0 Task longer than Ts (expected for N = 64 at 
100 µs)
Valid result. For the sweep add a run with 
CONTROL_PERIOD_US = 200; for the MMC 
task reduce the work or lengthen Ts.
Period avg ≠ 17 000 while 
overruns = 0
f_CPU ≠ 170 MHz or TIM6 prescaler 
assumption broken
Run E0b; check 
CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC 
in .config.<PARSED TEXT FOR PAGE: 11 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 11 of 21
10. Complete source code — main.cpp (v2)
Drop-in replacement for src/main.cpp. Compiled warning-free on host with GCC -std=c++20 -Wall -Wextra 
-Wpedantic -Wvolatile for all 12 combinations of TIMING_TEST_MODE × TIMING_USE_STM32_LL × 
TIMING_IRQ_SOURCE_TIM6, and functionally exercised with emulated DWT/TIM6/GPIO (ISR body × 3000, 
reporter × 3). The hardware build is done in your OwnTech project (Section 7.1).
/*
 * =============================================================================
 * OwnTech SPIN / TWIST -- Critical-task timing benchmark (v2, reviewed)
 * =============================================================================
 *
 * Three independent measurements of the same critical task:
 * (A) DWT_CYCCNT : CPU-cycle counter, 1-cycle resolution (internal)
 * (B) GPIO pulse : HIGH while the target runs, measured on an oscilloscope
 * (C) TIM6->CNT : read at task entry = interrupt-entry latency (0.1 us)
 * plus the actual task period (jitter) and an overrun counter.
 *
 * Target : STM32G474RE, Cortex-M4F @ 170 MHz (SPIN 1.2.0), OwnTech Core,
 * Zephyr RTOS, PlatformIO.
 *
 * Facts verified against the OwnTech Core sources (github owntech-foundation/core):
 * - GpioHAL: configurePin/setPin/resetPin take a uint8_t. SPIN pin 9 maps to
 * GPIOC bit 7 (PC7). It is the pin used in OwnTech's own GPIO example and
 * it is not reserved by the TWIST v1.4.x shield overlay. (Do not start the
 * TIM3 incremental encoder: it would claim PC6/PC7.)
 * - TaskAPI::createCritical(fn, period_us, source_tim6): 1..6553 us. The task
 * is executed INSIDE the TIM6 UPDATE interrupt, registered with
 * IRQ_ZERO_LATENCY. It is an ISR: no printk, no k_* calls, no blocking.
 * - TaskAPI::startCritical(false) is mandatory here. With TIM6 and the
 * default argument (true), the scheduler reads the HRTIM period, which is
 * 0 when PWM is not initialised, and returns WITHOUT starting the task.
 * - CPU clock: spin.dts sets rcc clock-frequency = 170 MHz; Zephyr exposes
 * it as CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC. The OwnTech TIM6 driver
 * derives its 0.1 us tick from the same constant (prescaler = f/1e7 - 1).
 * - Console: USB CDC-ACM (zephyr,console = cdc_acm_uart0), 115200 baud.
 *
 * Safety: the power stage, PWM outputs and ADC acquisition are never enabled.
 *
 * Quick start:
 * 1. TIMING_TEST_MODE = 0 -> E0 (instrumentation overhead)
 * 2. TIMING_TEST_MODE = 2 -> E0b (10 us reference: validates f_CPU on scope)
 * 3. TIMING_TEST_MODE = 1 -> E1..E5, sweep TEST_SORT_N = 4, 8, 16, 32, 64
 * 4. Replace task_under_test() by the MMC algorithm -> E6
 * =============================================================================
 */
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "SpinAPI.h"
#include "TaskAPI.h"
/* ============================ User configuration ============================ */
/* Control period Ts in microseconds. TIM6 source allows 1..6553 us. */
#define CONTROL_PERIOD_US 100U
/* 1 = TIM6 triggers the critical task (default for E0..E6, no PWM needed).
 * 0 = HRTIM master timer triggers it (E7, full loop): PWM must be initialised
 * BEFORE createCritical() and Ts must be a multiple of the PWM period.
 * Entry-latency measurement (TIM6->CNT) is only meaningful with TIM6. */
#define TIMING_IRQ_SOURCE_TIM6 1
/* Oscilloscope pin. SPIN header pin number (1..58) or STM32 name (PC7...).
 * Default 9 = PC7: free on TWIST, used in OwnTech's GPIO example. */<PARSED TEXT FOR PAGE: 12 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 12 of 21
#define TIMING_GPIO_PIN 9U
/* Executions per reported batch. 1000 @ Ts=100us -> one report every 100 ms. */
#define BATCH_SAMPLES 1000U
/* Bubble-sort workload size (E1..E5 sweep: 4, 8, 16, 32, 64). */
#define MAX_SORT_N 64U
#define TEST_SORT_N 16U
/* Reference busy-wait length for TIMING_TEST_MODE 2 (clock cross-check). */
#define REFERENCE_DELAY_US 10U
/* 0 = empty window : instrumentation calibration (E0)
 * 1 = bubble sort : worst-case (reverse-sorted) input (E1..E5)
 * 2 = reference delay : DWT-timed REFERENCE_DELAY_US busy-wait (E0b)
 * -> scope must show REFERENCE_DELAY_US + E0 offset.
 * If not, f_CPU is not what CPU_FREQ_HZ says. */
#define TIMING_TEST_MODE 1
/* 1 = STM32 LL register access (recommended):
 * - GPIO edges written directly to BSRR/BRR (~1 cycle instead of the
 * ~100-200 cycles of the generic Zephyr GPIO path),
 * - TIM6->CNT read at task entry = interrupt-entry latency.
 * 0 = portable path: OwnTech spin.gpio API only, no latency measurement.
 * If <stm32_ll_gpio.h> is not found by your build, set this to 0. */
#define TIMING_USE_STM32_LL 1
/* 1 = also print one machine-readable "CSV,..." line per batch. */
#define PRINT_CSV 1
/* ============================ Derived constants ============================= */
#ifdef CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#define CPU_FREQ_HZ ((uint32_t)CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)
#else
#define CPU_FREQ_HZ 170000000UL
#endif
/* TIM6 ticks at 0.1 us (OwnTech timer driver). Cycles per tick = f/10 MHz. */
#define TIM6_TICK_HZ 10000000UL
#define CYCLES_PER_TIM6_TICK (CPU_FREQ_HZ / TIM6_TICK_HZ)
#if TEST_SORT_N > MAX_SORT_N
#error "TEST_SORT_N must be <= MAX_SORT_N"
#endif
#if (CONTROL_PERIOD_US < 1U) || (CONTROL_PERIOD_US > 6553U)
#error "CONTROL_PERIOD_US must be within 1..6553 us for the TIM6 source"
#endif
#if (TIMING_TEST_MODE < 0) || (TIMING_TEST_MODE > 2)
#error "Unsupported TIMING_TEST_MODE"
#endif
#if TIMING_IRQ_SOURCE_TIM6
#define TIMING_IRQ_SOURCE source_tim6
#else
#define TIMING_IRQ_SOURCE source_hrtim
#endif
/* Entry latency is read from TIM6->CNT, so it needs LL access AND TIM6 source. */
#define MEASURE_ENTRY_LATENCY (TIMING_USE_STM32_LL && TIMING_IRQ_SOURCE_TIM6)
#if TIMING_USE_STM32_LL
#include <stm32_ll_gpio.h>
#include <stm32_ll_tim.h>
#endif
/* ======================= Cortex-M4 DWT / CoreDebug ========================== */<PARSED TEXT FOR PAGE: 13 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 13 of 21
/* Standard Cortex-M4 addresses (ARM DDI0439, ST PM0214). No CMSIS needed.
 * The #ifndef guards allow a host-side unit test to redirect the registers. */
#ifndef COREDEBUG_DEMCR
#define COREDEBUG_DEMCR (*(volatile uint32_t *)0xE000EDFCUL)
#endif
#ifndef DWT_CTRL_REG
#define DWT_CTRL_REG (*(volatile uint32_t *)0xE0001000UL)
#endif
#ifndef DWT_CYCCNT_REG
#define DWT_CYCCNT_REG (*(volatile uint32_t *)0xE0001004UL)
#endif
#define COREDEBUG_TRCENA (1UL << 24)
#define DWT_CYCCNTENA (1UL << 0)
#define DWT_NOCYCCNT (1UL << 25) /* read-only: 1 = no cycle counter */
/* ============================ SPIN pin mapping ============================== */
/* Replica of GpioHAL::getPinNumber()/getGpioDevice() (OwnTech Core), so that
 * the LL fast path and the start-up banner always agree with TIMING_GPIO_PIN.
 * port: 0 = GPIOA, 1 = GPIOB, 2 = GPIOC, 3 = GPIOD */
struct spin_pin_map_t
{
 uint8_t spin_pin;
 uint8_t port;
 uint8_t bit;
};
static constexpr spin_pin_map_t k_spin_pin_map[] =
{
 { 1, 1, 11}, { 2, 1, 12}, { 4, 1, 13}, { 5, 1, 14}, { 6, 1, 15},
 { 7, 2, 6}, { 9, 2, 7}, {10, 2, 8}, {11, 2, 9}, {12, 0, 8},
 {14, 0, 9}, {15, 0, 10}, {16, 2, 10}, {17, 2, 11}, {19, 2, 12},
 {20, 1, 4}, {21, 1, 9}, {22, 2, 13}, {24, 2, 0}, {25, 2, 1},
 {26, 2, 2}, {27, 2, 3}, {29, 0, 0}, {30, 0, 1}, {31, 1, 0},
 {32, 0, 5}, {34, 0, 6}, {35, 2, 4}, {37, 1, 1}, {41, 1, 10},
 {42, 1, 2}, {43, 2, 5}, {44, 0, 7}, {45, 0, 4}, {46, 0, 13},
 {47, 0, 14}, {48, 0, 15}, {49, 3, 2}, {50, 1, 3}, {51, 0, 2},
 {52, 0, 3}, {53, 1, 5}, {55, 1, 6}, {56, 1, 7}, {58, 1, 8}
};
/* Returns port index (0..3) or -1. Accepts SPIN numbers and PA0..PD3 codes. */
static constexpr int spin_pin_port(uint8_t pin)
{
 if ((pin & 0x80U) != 0U)
 {
 return (int)((pin >> 4) & 0x03U); /* 0x8x=A 0x9x=B 0xAx=C 0xBx=D */
 }
 for (const spin_pin_map_t &e : k_spin_pin_map)
 {
 if (e.spin_pin == pin) return (int)e.port;
 }
 return -1;
}
static constexpr int spin_pin_bit(uint8_t pin)
{
 if ((pin & 0x80U) != 0U)
 {
 return (int)(pin & 0x0FU);
 }
 for (const spin_pin_map_t &e : k_spin_pin_map)
 {
 if (e.spin_pin == pin) return (int)e.bit;
 }
 return -1;
}
static constexpr int k_timing_port = spin_pin_port((uint8_t)TIMING_GPIO_PIN);
static constexpr int k_timing_bit = spin_pin_bit((uint8_t)TIMING_GPIO_PIN);<PARSED TEXT FOR PAGE: 14 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 14 of 21
static_assert(k_timing_port >= 0 && k_timing_bit >= 0,
 "TIMING_GPIO_PIN is not a GPIO-capable SPIN pin (see GpioHAL.cpp)");
static_assert(!(k_timing_port == 0 && (k_timing_bit == 13 || k_timing_bit == 14)),
 "PA13/PA14 are SWDIO/SWCLK: do not use them as the timing pin");
#if TIMING_USE_STM32_LL
static constexpr uintptr_t k_timing_port_base =
 (k_timing_port == 0) ? (uintptr_t)GPIOA_BASE :
 (k_timing_port == 1) ? (uintptr_t)GPIOB_BASE :
 (k_timing_port == 2) ? (uintptr_t)GPIOC_BASE : (uintptr_t)GPIOD_BASE;
#define TIMING_LL_PORT ((GPIO_TypeDef *)k_timing_port_base)
#define TIMING_LL_MASK (1UL << (uint32_t)k_timing_bit)
#endif
/* ============================ Timing statistics ============================= */
typedef struct
{
 uint32_t min_cycles;
 uint32_t max_cycles;
 uint64_t sum_cycles;
 uint32_t count;
} timing_accumulator_t;
typedef struct
{
 uint32_t min;
 uint32_t avg;
 uint32_t max;
 uint32_t count;
} timing_summary_t;
/* Written by the ISR, read by the background thread (seqlock on batch_id). */
typedef struct
{
 volatile uint32_t batch_id;
 volatile timing_summary_t target; /* task_under_test() only */
 volatile timing_summary_t critical; /* whole user callback body */
 volatile timing_summary_t latency; /* TIM6 event -> callback entry */
 volatile timing_summary_t period; /* entry-to-entry (jitter) */
 volatile uint32_t overruns; /* body >= Ts in this batch */
 volatile uint32_t overruns_total; /* since boot */
} timing_snapshot_t;
static timing_accumulator_t g_target_acc;
static timing_accumulator_t g_critical_acc;
static timing_accumulator_t g_latency_acc;
static timing_accumulator_t g_period_acc;
static uint32_t g_overruns_batch = 0U;
static uint32_t g_overruns_total = 0U;
static uint32_t g_prev_start = 0U;
static bool g_have_prev = false;
static uint32_t g_budget_cycles = 0U;
static timing_snapshot_t g_snapshot;
#if TIMING_TEST_MODE == 1
/* Workload data. Replace with the real MMC algorithm for E6. */
static float g_sort_data[MAX_SORT_N];
static volatile float g_result_guard = 0.0F;
static uint32_t g_pattern_counter = 0U;
#endif
/* ================================ Helpers =================================== */
static inline void compiler_barrier(void)
{
 __asm__ volatile ("" ::: "memory");
}<PARSED TEXT FOR PAGE: 15 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 15 of 21
static inline uint32_t dwt_now(void)
{
 return DWT_CYCCNT_REG;
}
/* Returns true if the cycle counter exists and is counting. */
static bool dwt_init(void)
{
 /* No compound assignment on volatile: deprecated in C++20 (-Wvolatile). */
 COREDEBUG_DEMCR = COREDEBUG_DEMCR | COREDEBUG_TRCENA;
 compiler_barrier();
 if ((DWT_CTRL_REG & DWT_NOCYCCNT) != 0U)
 {
 return false;
 }
 DWT_CYCCNT_REG = 0U;
 DWT_CTRL_REG = DWT_CTRL_REG | DWT_CYCCNTENA;
 compiler_barrier();
 const uint32_t a = dwt_now();
 for (uint32_t i = 0U; i < 50U; ++i)
 {
 compiler_barrier(); /* keeps the loop, no volatile needed */
 }
 const uint32_t b = dwt_now();
 return (b != a);
}
static void stats_reset(timing_accumulator_t *s)
{
 s->min_cycles = UINT32_MAX;
 s->max_cycles = 0U;
 s->sum_cycles = 0U;
 s->count = 0U;
}
static inline void stats_add(timing_accumulator_t *s, uint32_t cycles)
{
 if (cycles < s->min_cycles) s->min_cycles = cycles;
 if (cycles > s->max_cycles) s->max_cycles = cycles;
 s->sum_cycles += cycles;
 s->count++;
}
static void stats_publish(volatile timing_summary_t *dst,
 const timing_accumulator_t *src)
{
 dst->min = (src->count != 0U) ? src->min_cycles : 0U;
 dst->max = src->max_cycles;
 dst->avg = (src->count != 0U) ? (uint32_t)(src->sum_cycles / src->count) : 0U;
 dst->count = src->count;
}
static void summary_copy(timing_summary_t *dst, const volatile timing_summary_t *src)
{
 dst->min = src->min;
 dst->avg = src->avg;
 dst->max = src->max;
 dst->count = src->count;
}
static uint64_t cycles_to_ns(uint32_t cycles)
{
 return ((uint64_t)cycles * 1000000000ULL) / (uint64_t)CPU_FREQ_HZ;
}<PARSED TEXT FOR PAGE: 16 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 16 of 21
/* "%u cycles (%u.%03u us)" -- avoids 64-bit printk formats. */
static void print_cycles(uint32_t cycles)
{
 const uint64_t ns = cycles_to_ns(cycles);
 printk("%6u cyc = %4u.%03u us",
 cycles, (uint32_t)(ns / 1000ULL), (uint32_t)(ns % 1000ULL));
}
static void print_summary(const char *label, const timing_summary_t *s)
{
 printk(" %-14s min ", label); print_cycles(s->min);
 printk(" avg "); print_cycles(s->avg);
 printk(" max "); print_cycles(s->max);
 printk("\n");
}
/* ============================ GPIO edge helpers ============================= */
static inline void timing_pin_high(void)
{
#if TIMING_USE_STM32_LL
 LL_GPIO_SetOutputPin(TIMING_LL_PORT, TIMING_LL_MASK);
#else
 spin.gpio.setPin((uint8_t)TIMING_GPIO_PIN);
#endif
}
static inline void timing_pin_low(void)
{
#if TIMING_USE_STM32_LL
 LL_GPIO_ResetOutputPin(TIMING_LL_PORT, TIMING_LL_MASK);
#else
 spin.gpio.resetPin((uint8_t)TIMING_GPIO_PIN);
#endif
}
/* ============================ Benchmark workloads =========================== */
#if TIMING_TEST_MODE == 1
static void prepare_sort_input(void)
{
 /* Reverse-sorted input = worst case for bubble sort (N(N-1)/2 compares
 * and swaps, early-exit never taken). Runs OUTSIDE the target window.
 * The small varying offset defeats any cross-iteration constant folding. */
 const float offset = (float)(g_pattern_counter & 0x7U) * 0.001F;
 for (uint32_t i = 0U; i < TEST_SORT_N; ++i)
 {
 g_sort_data[i] = (float)(TEST_SORT_N - i) + offset;
 }
 g_pattern_counter++;
}
__attribute__((noinline)) static void bubble_sort_task(void)
{
 for (uint32_t i = 0U; i + 1U < TEST_SORT_N; ++i)
 {
 bool swapped = false;
 for (uint32_t j = 0U; j + 1U < TEST_SORT_N - i; ++j)
 {
 if (g_sort_data[j] > g_sort_data[j + 1U])
 {
 const float tmp = g_sort_data[j];
 g_sort_data[j] = g_sort_data[j + 1U];
 g_sort_data[j + 1U] = tmp;
 swapped = true;
 }
 }
 if (!swapped) break;<PARSED TEXT FOR PAGE: 17 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 17 of 21
 }
}
#endif /* TIMING_TEST_MODE == 1 */
#if TIMING_TEST_MODE == 2
/* Busy-waits exactly REFERENCE_DELAY_US according to DWT. The oscilloscope
 * must then show REFERENCE_DELAY_US + (E0 pulse overhead). A systematic
 * deviation means CPU_FREQ_HZ does not match the real core clock. */
__attribute__((noinline)) static void reference_delay_task(void)
{
 const uint32_t start = dwt_now();
 const uint32_t wait = REFERENCE_DELAY_US * (CPU_FREQ_HZ / 1000000UL);
 while ((dwt_now() - start) < wait)
 {
 compiler_barrier();
 }
}
#endif /* TIMING_TEST_MODE == 2 */
__attribute__((noinline)) static void task_under_test(void)
{
#if TIMING_TEST_MODE == 0
 compiler_barrier(); /* empty: instrumentation overhead only */
#elif TIMING_TEST_MODE == 1
 bubble_sort_task(); /* <- replace by the MMC algorithm (E6) */
#else
 reference_delay_task();
#endif
}
/* ============================= Snapshot (ISR side) ========================== */
static void publish_snapshot_if_ready(void)
{
 if (g_target_acc.count < BATCH_SAMPLES) return;
 stats_publish(&g_snapshot.target, &g_target_acc);
 stats_publish(&g_snapshot.critical, &g_critical_acc);
 stats_publish(&g_snapshot.latency, &g_latency_acc);
 stats_publish(&g_snapshot.period, &g_period_acc);
 g_snapshot.overruns = g_overruns_batch;
 g_snapshot.overruns_total = g_overruns_total;
 /* All fields complete before the id changes. The writer is an ISR that the
 * reader thread can never interrupt, so a single counter is sufficient. */
 compiler_barrier();
 g_snapshot.batch_id = g_snapshot.batch_id + 1U;
 stats_reset(&g_target_acc);
 stats_reset(&g_critical_acc);
 stats_reset(&g_latency_acc);
 stats_reset(&g_period_acc);
 g_overruns_batch = 0U;
}
/* ====================== Critical task (TIM6 ISR context) ==================== */
static void loop_critical_task(void)
{
#if MEASURE_ENTRY_LATENCY
 /* TIM6 counts 0.1 us ticks from 0 at each UPDATE event: CNT here is the
 * time from the timer event to the first instruction of this function
 * (NVIC entry + Zephyr ISR wrapper + OwnTech proxy). */
 const uint32_t entry_ticks = LL_TIM_GetCounter(TIM6);
#endif
 const uint32_t critical_start = dwt_now();
#if TIMING_TEST_MODE == 1<PARSED TEXT FOR PAGE: 18 / 21>OwnTech SPIN/TWIST timing benchmark — Experiment Setup Guide
Page 18 of 21
 prepare_sort_input();
#endif
 /* ---- measured target: GPIO HIGH ... LOW brackets the DWT window ---- */
 timing_pin_high();
 compiler_barrier();
 const uint32_t target_start = dwt_now();
 task_under_test();
 compiler_barrier();
 const uint32_t target_end = dwt_now();
 timing_pin_low();
 /* -------------------------------------------------------------------- */
#if TIMING_TEST_MODE == 1
 /* Visible side effect so the optimiser cannot discard the sort. */
 g_result_guard = g_sort_data[0] + g_sort_data[TEST_SORT_N - 1U];
#endif
 const uint32_t critical_end = dwt_now();
 /* Unsigned differences are wrap-safe for intervals < 2^32 cycles (25 s). */
 const uint32_t target_cycles = target_end - target_start;
 const uint32_t critical_cycles = critical_end - critical_start;
 stats_add(&g_target_acc, target_cycles);
 stats_add(&g_critical_acc, critical_cycles);
#if MEASURE_ENTRY_LATENCY
 stats_add(&g_latency_acc, entry_ticks * (uint32_t)CYCLES_PER_TIM6_TICK);
#endif
 if (g_have_prev)
 {
 stats_add(&g_period_acc, critical_start - g_prev_start);
 }
 g_prev_start = critical_start;
