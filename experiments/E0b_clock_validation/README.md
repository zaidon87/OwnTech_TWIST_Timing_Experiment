# E0b - Clock validation

Set:

```text
TIMING_TEST_MODE   = 2
REFERENCE_DELAY_US = 10
```

The DWT loop is intentionally constructed around a 10 us reference. The oscilloscope provides the independent time reference after subtracting the E0 scope offset.

If the corrected scope duration disagrees materially with the assumed CPU clock, investigate the clock tree before interpreting the remaining DWT-derived times.
