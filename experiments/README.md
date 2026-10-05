# Experiments E0-E7

The campaign is intentionally incremental. Do not start by benchmarking the real MMC controller before validating the timing instrumentation.

| ID | Target | Main configuration | Output |
|---|---|---|---|
| E0 | empty target | mode 0 | instrumentation overhead and scope offset |
| E0b | 10 us reference | mode 2 | clock validation |
| E1 | bubble sort N=4 | mode 1 | baseline |
| E2 | bubble sort N=8 | mode 1 | baseline |
| E3 | bubble sort N=16 | mode 1 | baseline |
| E4 | bubble sort N=32 | mode 1 | baseline |
| E5 | bubble sort N=64 | mode 1 | stress / possible overrun at 100 us |
| E6 | MMC algorithm | manual integration | real algorithm timing |
| E7 | full loop | HRTIM or required synchronized source | end-to-end control timing |

For each experiment, save the console log and the oscilloscope measurement record under `results/`.
