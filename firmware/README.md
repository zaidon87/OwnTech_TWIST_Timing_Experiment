# Firmware

`src/main.cpp` is the reviewed benchmark source supplied with this repository.

The recommended execution model is to copy this file into an **existing OwnTech project that already builds an official example**. This avoids pretending that the small configuration snippets in this repository replace the complete OwnTech Core environment.

## Important compile-time controls

The key macros are near the top of `main.cpp`:

```cpp
#define CONTROL_PERIOD_US        100U
#define TIMING_IRQ_SOURCE_TIM6   1
#define TIMING_GPIO_PIN          9U
#define BATCH_SAMPLES            1000U
#define MAX_SORT_N               64U
#define TEST_SORT_N              16U
#define REFERENCE_DELAY_US       10U
#define TIMING_TEST_MODE         1
#define TIMING_USE_STM32_LL      1
#define PRINT_CSV                1
```

The `scripts/select_experiment.py` helper modifies only `TIMING_TEST_MODE`, `TEST_SORT_N`, `CONTROL_PERIOD_US`, and `TIMING_IRQ_SOURCE_TIM6` for E0-E5/E0b.

## Timing-critical code policy

Avoid refactoring the DWT/GPIO window merely for code style. Function boundaries, inlining, compiler optimization, GPIO access path, and input preparation can all change measured timing. Any firmware refactor should be treated as a new benchmark version and re-calibrated from E0.
