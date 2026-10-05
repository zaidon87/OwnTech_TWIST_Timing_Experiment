# E7 - Full control loop

E7 extends the timing window to the end-to-end path required by the real control application, for example:

```text
acquisition read -> control -> sorting/balancing -> communication -> PWM update
```

If the task must be synchronized with PWM, use the HRTIM source and initialize the required PWM/ADC path before creating the critical task. The TIM6 entry-latency measurement is then not meaningful and is reported as unavailable, while target/body timing, period, and overruns remain useful.

This is a powered-system experiment. Follow the laboratory's power-up, isolation, and probing procedures.
