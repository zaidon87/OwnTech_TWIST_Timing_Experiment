# Measurement Method

## 1. Internal target timing: DWT_CYCCNT

The firmware reads the Cortex-M4 DWT cycle counter immediately before and after `task_under_test()`.

```text
T = target_end - target_start
```

At 170 MHz, one cycle corresponds to approximately 5.882 ns.

## 2. Critical-body timing

The callback body is timed from its first DWT timestamp to the final body timestamp, excluding the statistics/snapshot work that follows.

```text
C = critical_end - critical_start
```

## 3. Interrupt-entry latency

With the TIM6 trigger and STM32 LL access enabled, TIM6->CNT is read at callback entry. The timer runs at 10 MHz, so one tick is 0.1 us. At 170 MHz this is 17 CPU cycles per TIM6 tick.

```text
L = TIM6_CNT_at_entry * cycles_per_TIM6_tick
```

## 4. Period and jitter

The actual period is measured entry-to-entry with DWT:

```text
period[n] = critical_start[n] - critical_start[n-1]
jitter    = period_max - period_min
```

## 5. External oscilloscope validation

The timing pin is set HIGH just before the DWT target start and LOW just after the DWT target end. Therefore the oscilloscope pulse includes a small GPIO/instrumentation offset.

Calibrate it with E0:

```text
delta = P_E0 - T_E0
```

Then use:

```text
T_scope_corrected = P - delta
error_percent = abs(T_scope_corrected - T_DWT) / T_DWT * 100
```

For very short pulses, absolute error is more meaningful than relative percent error.

## 6. Deadline quantities

```text
Worst observed time = L_max + C_max
Slack               = Ts - (L_max + C_max)
Utilisation         = (L_max + C_max) / Ts * 100 %
```

The firmware declares PASS when there is no overrun in the batch and `L_max + C_max < Ts`.

## Terminology

The observed maxima are measurement results over the collected samples. They are not formal WCET bounds unless a separate static/WCET analysis is performed.
