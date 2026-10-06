#pragma once
// Modes: 0 empty, 2 reference delay, 3 centralized CVB (1 reserved for baseline).
#define TIMING_TEST_MODE         3
// CVB scope: 0 both arms, 1 upper only, 2 lower only.
#define CVB_SCOPE                0
#define CONTROL_PERIOD_US        200U
#define TIMING_IRQ_SOURCE_TIM6   1
#define TIMING_GPIO_PIN          9U
#define BATCH_SAMPLES            1000U
#define MAX_SORT_N               64U
#define CVB_MODULES_PER_ARM      10U
#define TEST_SORT_N              CVB_MODULES_PER_ARM
#define REFERENCE_DELAY_US       10U
#define TIMING_USE_STM32_LL      1
#define TIMING_VERBOSE_REPORT   0
#define PRINT_CSV                1
static_assert(CVB_SCOPE >= 0 && CVB_SCOPE <= 2, "Invalid CVB scope");

static_assert(CVB_MODULES_PER_ARM == 5U || CVB_MODULES_PER_ARM == 10U, "Validated sizes: 5 or 10 per arm");
