# E6 - Real MMC algorithm

E6 is intentionally not auto-generated because it depends on the actual control function being studied.

## Integration rule

Replace the body of `task_under_test()` with a call to the real algorithm, for example a local sorting or capacitor-voltage balancing step.

Keep these outside the target window when they are not logically part of the algorithm being measured:

- construction of synthetic test inputs;
- logging;
- report formatting;
- background communication used only to export the result.

Keep a visible side effect after the target so the compiler cannot delete the calculation.

## Input patterns

Run difficult, defined patterns as separate sub-experiments rather than mixing them silently. The supplied guide gives examples such as reverse-sorted voltages, all-equal voltages, and alternating patterns. Report the largest observed maximum together with the exact input pattern and build configuration.
