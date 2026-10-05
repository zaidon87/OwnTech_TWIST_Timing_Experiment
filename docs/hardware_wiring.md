# Hardware Wiring

## E0-E6

Use USB only. Do not energize the power stage.

```text
PC / USB-C
    |
    +----> SPIN 1.2.0 (+ TWIST shield may remain mounted)

Oscilloscope CH1 tip  ----> SPIN header pin 9 = STM32 PC7
Oscilloscope ground   ----> SPIN logic GND
```

Recommended probe arrangement:

- 10x passive probe;
- DC coupling;
- short ground spring/lead;
- trigger on CH1 rising edge near the middle of the 3.3 V logic level.

At `Ts = 100 us`, check that the pulse repetition is approximately `10.00 kHz / 100.0 us`.

## Pin caution

PC7 is the reviewed default timing pin. Do not start the TIM3 incremental encoder during the benchmark because its pin configuration can claim PC6/PC7.

The full experiment guide contains the complete SPIN-to-STM32 pin table and alternate timing-pin candidates.

## E7

E7 can involve an energized converter. Use the laboratory's required isolation, grounding, and differential/isolated probing method. Do not assume the logic ground is earth-referenced in every converter configuration.
