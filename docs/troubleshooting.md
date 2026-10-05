# Troubleshooting

## No console output

Check the SPIN USB CDC virtual COM port, data-capable USB cable, and 115200 monitor setting.

## Banner appears but there are no batch reports

For E0-E6 with TIM6 and no PWM, keep:

```cpp
task.startCritical(false);
```

Using the default `true` path can make startup depend on an HRTIM period that is not initialized.

## No oscilloscope pulse

Check that the probe is on SPIN pin 9 / PC7, the logic ground is connected, and no code has initialized a peripheral that claims PC7.

## Pulse is much longer than DWT target time

Check `TIMING_USE_STM32_LL`. With the portable OwnTech/Zephyr GPIO path, GPIO software overhead is expected to be much larger. Calibrate it with E0 or use the LL path when supported.

## Period jitter or entry latency is unexpectedly large

Disable unrelated high-priority/zero-latency activity during the isolated benchmark, for example communication or acquisition interrupts that are not part of the experiment. Record every enabled module.

## N=64 misses the 100 us deadline

Treat that as a valid benchmark result. The experiment guide explicitly allows an additional E5 run at a longer period such as 200 us, provided that change is documented rather than hidden.

## LL headers are not available

Set:

```cpp
#define TIMING_USE_STM32_LL 0
```

The portable path remains usable, but TIM6 entry-latency measurement becomes unavailable and GPIO pulse overhead increases.
