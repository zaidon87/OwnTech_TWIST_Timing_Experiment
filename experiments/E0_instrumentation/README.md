# E0 - Instrumentation calibration

Set:

```text
TIMING_TEST_MODE       = 0
TIMING_USE_STM32_LL    = 1
TIMING_IRQ_SOURCE_TIM6 = 1
CONTROL_PERIOD_US      = 100
```

Purpose: measure the empty target window, critical-body baseline, entry latency, period jitter, and oscilloscope pulse overhead.

Use the scope measurements to compute:

```text
delta = P_E0 - T_E0
```

Use mean with mean and max with max consistently when cross-checking later experiments.
