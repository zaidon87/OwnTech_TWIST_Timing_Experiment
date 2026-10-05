#pragma once
// Adapted from timing_v2; MIT license in benchmarks/cvb/reference/TIMING_LICENSE.
/*
 * =============================================================================
 *  OwnTech SPIN / TWIST -- Critical-task timing benchmark   (v2, reviewed)
 * =============================================================================
 *
 *  Three independent measurements of the same critical task:
 *   (A) DWT_CYCCNT  : CPU-cycle counter, 1-cycle resolution (internal)
 *   (B) GPIO pulse  : HIGH while the target runs, measured on an oscilloscope
 *   (C) TIM6->CNT   : read at task entry = interrupt-entry latency (0.1 us)
 *  plus the actual task period (jitter) and an overrun counter.
 *
 *  Target : STM32G474RE, Cortex-M4F @ 170 MHz (SPIN 1.2.0), OwnTech Core,
 *           Zephyr RTOS, PlatformIO.
 *
 *  Facts verified against the OwnTech Core sources (github owntech-foundation/core):
 *   - GpioHAL: configurePin/setPin/resetPin take a uint8_t. SPIN pin 9 maps to
 *     GPIOC bit 7 (PC7). It is the pin used in OwnTech's own GPIO example and
 *     it is not reserved by the TWIST v1.4.x shield overlay. (Do not start the
 *     TIM3 incremental encoder: it would claim PC6/PC7.)
 *   - TaskAPI::createCritical(fn, period_us, source_tim6): 1..6553 us. The task
 *     is executed INSIDE the TIM6 UPDATE interrupt, registered with
 *     IRQ_ZERO_LATENCY. It is an ISR: no printk, no k_* calls, no blocking.
 *   - TaskAPI::startCritical(false) is mandatory here. With TIM6 and the
 *     default argument (true), the scheduler reads the HRTIM period, which is
 *     0 when PWM is not initialised, and returns WITHOUT starting the task.
 *   - CPU clock: spin.dts sets rcc clock-frequency = 170 MHz; Zephyr exposes
 *     it as CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC. The OwnTech TIM6 driver
 *     derives its 0.1 us tick from the same constant (prescaler = f/1e7 - 1).
 *   - Console: USB CDC-ACM (zephyr,console = cdc_acm_uart0), 115200 baud.
 *
 *  Safety: the power stage, PWM outputs and ADC acquisition are never enabled.
 *
 *  Quick start:
 *   1. TIMING_TEST_MODE = 0  -> E0  (instrumentation overhead)
 *   2. TIMING_TEST_MODE = 2  -> E0b (10 us reference: validates f_CPU on scope)
 *   3. TIMING_TEST_MODE = 1  -> E1..E5, sweep TEST_SORT_N = 4, 8, 16, 32, 64
 *   4. Replace task_under_test() by the MMC algorithm       -> E6
 * =============================================================================
 */

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "SpinAPI.h"
#include "TaskAPI.h"

#include "timing_config.h"

/* ============================ Derived constants ============================= */

#ifdef CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#define CPU_FREQ_HZ  ((uint32_t)CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)
#else
#define CPU_FREQ_HZ  170000000UL
#endif

/* TIM6 ticks at 0.1 us (OwnTech timer driver). Cycles per tick = f/10 MHz. */
#define TIM6_TICK_HZ             10000000UL
#define CYCLES_PER_TIM6_TICK     (CPU_FREQ_HZ / TIM6_TICK_HZ)

#if TEST_SORT_N > MAX_SORT_N
#error "TEST_SORT_N must be <= MAX_SORT_N"
#endif
#if (CONTROL_PERIOD_US < 1U) || (CONTROL_PERIOD_US > 6553U)
#error "CONTROL_PERIOD_US must be within 1..6553 us for the TIM6 source"
#endif
#if (TIMING_TEST_MODE < 0) || (TIMING_TEST_MODE > 3)
#error "Unsupported TIMING_TEST_MODE"
#endif

#if TIMING_IRQ_SOURCE_TIM6
#define TIMING_IRQ_SOURCE        source_tim6
#else
#define TIMING_IRQ_SOURCE        source_hrtim
#endif

/* Entry latency is read from TIM6->CNT, so it needs LL access AND TIM6 source. */
#define MEASURE_ENTRY_LATENCY    (TIMING_USE_STM32_LL && TIMING_IRQ_SOURCE_TIM6)

#if TIMING_USE_STM32_LL
#include <stm32_ll_gpio.h>
#include <stm32_ll_tim.h>
#endif

/* ======================= Cortex-M4 DWT / CoreDebug ========================== */
/* Standard Cortex-M4 addresses (ARM DDI0439, ST PM0214). No CMSIS needed.
 * The #ifndef guards allow a host-side unit test to redirect the registers. */
#ifndef COREDEBUG_DEMCR
#define COREDEBUG_DEMCR   (*(volatile uint32_t *)0xE000EDFCUL)
#endif
#ifndef DWT_CTRL_REG
#define DWT_CTRL_REG      (*(volatile uint32_t *)0xE0001000UL)
#endif
#ifndef DWT_CYCCNT_REG
#define DWT_CYCCNT_REG    (*(volatile uint32_t *)0xE0001004UL)
#endif
#define COREDEBUG_TRCENA  (1UL << 24)
#define DWT_CYCCNTENA     (1UL << 0)
#define DWT_NOCYCCNT      (1UL << 25)   /* read-only: 1 = no cycle counter */

/* ============================ SPIN pin mapping ============================== */
/* Replica of GpioHAL::getPinNumber()/getGpioDevice() (OwnTech Core), so that
 * the LL fast path and the start-up banner always agree with TIMING_GPIO_PIN.
 * port: 0 = GPIOA, 1 = GPIOB, 2 = GPIOC, 3 = GPIOD                            */
struct spin_pin_map_t
{
    uint8_t spin_pin;
    uint8_t port;
    uint8_t bit;
};

static constexpr spin_pin_map_t k_spin_pin_map[] =
{
    { 1, 1, 11}, { 2, 1, 12}, { 4, 1, 13}, { 5, 1, 14}, { 6, 1, 15},
    { 7, 2,  6}, { 9, 2,  7}, {10, 2,  8}, {11, 2,  9}, {12, 0,  8},
    {14, 0,  9}, {15, 0, 10}, {16, 2, 10}, {17, 2, 11}, {19, 2, 12},
    {20, 1,  4}, {21, 1,  9}, {22, 2, 13}, {24, 2,  0}, {25, 2,  1},
    {26, 2,  2}, {27, 2,  3}, {29, 0,  0}, {30, 0,  1}, {31, 1,  0},
    {32, 0,  5}, {34, 0,  6}, {35, 2,  4}, {37, 1,  1}, {41, 1, 10},
    {42, 1,  2}, {43, 2,  5}, {44, 0,  7}, {45, 0,  4}, {46, 0, 13},
    {47, 0, 14}, {48, 0, 15}, {49, 3,  2}, {50, 1,  3}, {51, 0,  2},
    {52, 0,  3}, {53, 1,  5}, {55, 1,  6}, {56, 1,  7}, {58, 1,  8}
};

/* Returns port index (0..3) or -1. Accepts SPIN numbers and PA0..PD3 codes. */
static constexpr int spin_pin_port(uint8_t pin)
{
    if ((pin & 0x80U) != 0U)
    {
        return (int)((pin >> 4) & 0x03U);      /* 0x8x=A 0x9x=B 0xAx=C 0xBx=D */
    }
    for (const spin_pin_map_t &e : k_spin_pin_map)
    {
        if (e.spin_pin == pin) return (int)e.port;
    }
    return -1;
}

static constexpr int spin_pin_bit(uint8_t pin)
{
    if ((pin & 0x80U) != 0U)
    {
        return (int)(pin & 0x0FU);
    }
    for (const spin_pin_map_t &e : k_spin_pin_map)
    {
        if (e.spin_pin == pin) return (int)e.bit;
    }
    return -1;
}

static constexpr int k_timing_port = spin_pin_port((uint8_t)TIMING_GPIO_PIN);
static constexpr int k_timing_bit  = spin_pin_bit((uint8_t)TIMING_GPIO_PIN);

static_assert(k_timing_port >= 0 && k_timing_bit >= 0,
              "TIMING_GPIO_PIN is not a GPIO-capable SPIN pin (see GpioHAL.cpp)");
static_assert(!(k_timing_port == 0 && (k_timing_bit == 13 || k_timing_bit == 14)),
              "PA13/PA14 are SWDIO/SWCLK: do not use them as the timing pin");

#if TIMING_USE_STM32_LL
static constexpr uintptr_t k_timing_port_base =
    (k_timing_port == 0) ? (uintptr_t)GPIOA_BASE :
    (k_timing_port == 1) ? (uintptr_t)GPIOB_BASE :
    (k_timing_port == 2) ? (uintptr_t)GPIOC_BASE : (uintptr_t)GPIOD_BASE;
#define TIMING_LL_PORT   ((GPIO_TypeDef *)k_timing_port_base)
#define TIMING_LL_MASK   (1UL << (uint32_t)k_timing_bit)
#endif

/* ============================ Timing statistics ============================= */

typedef struct
{
    uint32_t min_cycles;
    uint32_t max_cycles;
    uint64_t sum_cycles;
    uint32_t count;
} timing_accumulator_t;

typedef struct
{
    uint32_t min;
    uint32_t avg;
    uint32_t max;
    uint32_t count;
} timing_summary_t;

/* Written by the ISR, read by the background thread (seqlock on batch_id). */
typedef struct
{
    volatile uint32_t batch_id;
    volatile timing_summary_t target;     /* task_under_test() only        */
    volatile timing_summary_t critical;   /* whole user callback body      */
    volatile timing_summary_t latency;    /* TIM6 event -> callback entry  */
    volatile timing_summary_t period;     /* entry-to-entry (jitter)       */
    volatile uint32_t overruns;           /* body >= Ts in this batch      */
    volatile uint32_t overruns_total;     /* since boot                    */
} timing_snapshot_t;

static timing_accumulator_t g_target_acc;
static timing_accumulator_t g_critical_acc;
static timing_accumulator_t g_latency_acc;
static timing_accumulator_t g_period_acc;
static uint32_t             g_overruns_batch = 0U;
static uint32_t             g_overruns_total = 0U;
static uint32_t             g_prev_start     = 0U;
static bool                 g_have_prev      = false;
static uint32_t             g_budget_cycles  = 0U;
static timing_snapshot_t    g_snapshot;

#if TIMING_TEST_MODE == 1
/* Workload data. Replace with the real MMC algorithm for E6. */
static float          g_sort_data[MAX_SORT_N];
static volatile float g_result_guard = 0.0F;
static uint32_t       g_pattern_counter = 0U;
#endif

/* ================================ Helpers =================================== */

static inline void timing_compiler_barrier(void)
{
    __asm__ volatile ("" ::: "memory");
}

static inline uint32_t dwt_now(void)
{
    return DWT_CYCCNT_REG;
}

/* Returns true if the cycle counter exists and is counting. */
static bool dwt_init(void)
{
    /* No compound assignment on volatile: deprecated in C++20 (-Wvolatile). */
    COREDEBUG_DEMCR = COREDEBUG_DEMCR | COREDEBUG_TRCENA;
    timing_compiler_barrier();

    if ((DWT_CTRL_REG & DWT_NOCYCCNT) != 0U)
    {
        return false;
    }

    DWT_CYCCNT_REG = 0U;
    DWT_CTRL_REG = DWT_CTRL_REG | DWT_CYCCNTENA;
    timing_compiler_barrier();

    const uint32_t a = dwt_now();
    for (uint32_t i = 0U; i < 50U; ++i)
    {
        timing_compiler_barrier();        /* keeps the loop, no volatile needed */
    }
    const uint32_t b = dwt_now();
    return (b != a);
}

static void stats_reset(timing_accumulator_t *s)
{
    s->min_cycles = UINT32_MAX;
    s->max_cycles = 0U;
    s->sum_cycles = 0U;
    s->count = 0U;
}

static inline void stats_add(timing_accumulator_t *s, uint32_t cycles)
{
    if (cycles < s->min_cycles) s->min_cycles = cycles;
    if (cycles > s->max_cycles) s->max_cycles = cycles;
    s->sum_cycles += cycles;
    s->count++;
}

static void stats_publish(volatile timing_summary_t *dst,
                          const timing_accumulator_t *src)
{
    dst->min   = (src->count != 0U) ? src->min_cycles : 0U;
    dst->max   = src->max_cycles;
    dst->avg   = (src->count != 0U) ? (uint32_t)(src->sum_cycles / src->count) : 0U;
    dst->count = src->count;
}

static void summary_copy(timing_summary_t *dst, const volatile timing_summary_t *src)
{
    dst->min   = src->min;
    dst->avg   = src->avg;
    dst->max   = src->max;
    dst->count = src->count;
}

static uint64_t cycles_to_ns(uint32_t cycles)
{
    return ((uint64_t)cycles * 1000000000ULL) / (uint64_t)CPU_FREQ_HZ;
}

/* "%u cycles (%u.%03u us)" -- avoids 64-bit printk formats. */
static void print_cycles(uint32_t cycles)
{
    const uint64_t ns = cycles_to_ns(cycles);
    printk("%6u cyc = %4u.%03u us",
           cycles, (uint32_t)(ns / 1000ULL), (uint32_t)(ns % 1000ULL));
}

static void print_summary(const char *label, const timing_summary_t *s)
{
    printk("  %-14s min ", label); print_cycles(s->min);
    printk("   avg ");            print_cycles(s->avg);
    printk("   max ");            print_cycles(s->max);
    printk("\n");
}

/* ============================ GPIO edge helpers ============================= */

static inline void timing_pin_high(void)
{
#if TIMING_USE_STM32_LL
    LL_GPIO_SetOutputPin(TIMING_LL_PORT, TIMING_LL_MASK);
#else
    spin.gpio.setPin((uint8_t)TIMING_GPIO_PIN);
#endif
}

static inline void timing_pin_low(void)
{
#if TIMING_USE_STM32_LL
    LL_GPIO_ResetOutputPin(TIMING_LL_PORT, TIMING_LL_MASK);
#else
    spin.gpio.resetPin((uint8_t)TIMING_GPIO_PIN);
#endif
}

/* ============================ Benchmark workloads =========================== */

#if TIMING_TEST_MODE == 1
static void prepare_sort_input(void)
{
    /* Reverse-sorted input = worst case for bubble sort (N(N-1)/2 compares
     * and swaps, early-exit never taken). Runs OUTSIDE the target window.
     * The small varying offset defeats any cross-iteration constant folding. */
    const float offset = (float)(g_pattern_counter & 0x7U) * 0.001F;
    for (uint32_t i = 0U; i < TEST_SORT_N; ++i)
    {
        g_sort_data[i] = (float)(TEST_SORT_N - i) + offset;
    }
    g_pattern_counter++;
}

__attribute__((noinline)) static void bubble_sort_task(void)
{
    for (uint32_t i = 0U; i + 1U < TEST_SORT_N; ++i)
    {
        bool swapped = false;
        for (uint32_t j = 0U; j + 1U < TEST_SORT_N - i; ++j)
        {
            if (g_sort_data[j] > g_sort_data[j + 1U])
            {
                const float tmp     = g_sort_data[j];
                g_sort_data[j]      = g_sort_data[j + 1U];
                g_sort_data[j + 1U] = tmp;
                swapped = true;
            }
        }
        if (!swapped) break;
    }
}
#endif /* TIMING_TEST_MODE == 1 */

#if TIMING_TEST_MODE == 2
/* Busy-waits exactly REFERENCE_DELAY_US according to DWT. The oscilloscope
 * must then show REFERENCE_DELAY_US + (E0 pulse overhead). A systematic
 * deviation means CPU_FREQ_HZ does not match the real core clock.          */
__attribute__((noinline)) static void reference_delay_task(void)
{
    const uint32_t start = dwt_now();
    const uint32_t wait  = REFERENCE_DELAY_US * (CPU_FREQ_HZ / 1000000UL);
    while ((dwt_now() - start) < wait)
    {
        timing_compiler_barrier();
    }
}
#endif /* TIMING_TEST_MODE == 2 */

__attribute__((noinline)) static void task_under_test(void)
{
#if TIMING_TEST_MODE == 0
    timing_compiler_barrier();            /* empty: instrumentation overhead only */
#elif TIMING_TEST_MODE == 1
    bubble_sort_task();            /* <- replace by the MMC algorithm (E6) */
#elif TIMING_TEST_MODE == 2
    reference_delay_task();
#elif TIMING_TEST_MODE == 3
    cvb_execute();
#endif
}

/* ============================= Snapshot (ISR side) ========================== */

static void publish_snapshot_if_ready(void)
{
    if (g_target_acc.count < BATCH_SAMPLES) return;

    stats_publish(&g_snapshot.target,   &g_target_acc);
    stats_publish(&g_snapshot.critical, &g_critical_acc);
    stats_publish(&g_snapshot.latency,  &g_latency_acc);
    stats_publish(&g_snapshot.period,   &g_period_acc);
    g_snapshot.overruns       = g_overruns_batch;
    g_snapshot.overruns_total = g_overruns_total;

    /* All fields complete before the id changes. The writer is an ISR that the
     * reader thread can never interrupt, so a single counter is sufficient. */
    timing_compiler_barrier();
    g_snapshot.batch_id = g_snapshot.batch_id + 1U;

    stats_reset(&g_target_acc);
    stats_reset(&g_critical_acc);
    stats_reset(&g_latency_acc);
    stats_reset(&g_period_acc);
    g_overruns_batch = 0U;
}

/* ====================== Critical task (TIM6 ISR context) ==================== */

static void loop_critical_task(void)
{
#if MEASURE_ENTRY_LATENCY
    /* TIM6 counts 0.1 us ticks from 0 at each UPDATE event: CNT here is the
     * time from the timer event to the first instruction of this function
     * (NVIC entry + Zephyr ISR wrapper + OwnTech proxy). */
    const uint32_t entry_ticks = LL_TIM_GetCounter(TIM6);
#endif
    const uint32_t critical_start = dwt_now();

#if TIMING_TEST_MODE == 1
    prepare_sort_input();
#elif TIMING_TEST_MODE == 3
    prepare_cvb_input();
#endif

    /* ---- measured target: GPIO HIGH ... LOW brackets the DWT window ---- */
    timing_pin_high();
    timing_compiler_barrier();
    const uint32_t target_start = dwt_now();

    task_under_test();

    timing_compiler_barrier();
    const uint32_t target_end = dwt_now();
    timing_pin_low();
    /* -------------------------------------------------------------------- */

#if TIMING_TEST_MODE == 1
    /* Visible side effect so the optimiser cannot discard the sort. */
    g_result_guard = g_sort_data[0] + g_sort_data[TEST_SORT_N - 1U];
#elif TIMING_TEST_MODE == 3
    consume_cvb_result();
#endif

    const uint32_t critical_end = dwt_now();

    /* Unsigned differences are wrap-safe for intervals < 2^32 cycles (25 s). */
    const uint32_t target_cycles   = target_end   - target_start;
    const uint32_t critical_cycles = critical_end - critical_start;

    stats_add(&g_target_acc,   target_cycles);
    stats_add(&g_critical_acc, critical_cycles);

#if MEASURE_ENTRY_LATENCY
    stats_add(&g_latency_acc, entry_ticks * (uint32_t)CYCLES_PER_TIM6_TICK);
#endif

    if (g_have_prev)
    {
        stats_add(&g_period_acc, critical_start - g_prev_start);
    }
    g_prev_start = critical_start;
    g_have_prev  = true;

    if (critical_cycles >= g_budget_cycles)
    {
        g_overruns_batch++;
        g_overruns_total++;
    }

    publish_snapshot_if_ready();
}

/* ========================= Background reporting task ======================== */

static void loop_background_task(void)
{
    static uint32_t last_batch_id = 0U;

    uint32_t id_before;
    uint32_t id_after;
    timing_summary_t t, c, l, p;
    uint32_t overruns, overruns_total;

    do
    {
        id_before = g_snapshot.batch_id;
        timing_compiler_barrier();
        summary_copy(&t, &g_snapshot.target);
        summary_copy(&c, &g_snapshot.critical);
        summary_copy(&l, &g_snapshot.latency);
        summary_copy(&p, &g_snapshot.period);
        overruns       = g_snapshot.overruns;
        overruns_total = g_snapshot.overruns_total;
        timing_compiler_barrier();
        id_after = g_snapshot.batch_id;
    }
    while (id_before != id_after);

    if ((id_before != 0U) && (id_before != last_batch_id) && (t.count != 0U))
    {
        last_batch_id = id_before;

        const uint32_t budget = g_budget_cycles;
        /* Worst case seen by the CPU = entry latency + body (exit not included). */
        const uint32_t worst_isr = l.max + c.max;

        printk("\n=== Timing batch %u ===\n", id_before);
#if TIMING_TEST_MODE == 0
        printk("Mode: 0 EMPTY CALIBRATION");
#elif TIMING_TEST_MODE == 1
        printk("Mode: 1 BUBBLE SORT, N=%u", (uint32_t)TEST_SORT_N);
#elif TIMING_TEST_MODE == 3
        printk("Mode: 3 CENTRALIZED CVB, scope=%u, N=%u", (uint32_t)CVB_SCOPE, (uint32_t)TEST_SORT_N);
#else
        printk("Mode: 2 REFERENCE DELAY, %u us", (uint32_t)REFERENCE_DELAY_US);
#endif
        printk(" | Ts=%u us | f_CPU=%u Hz | pin %u (P%c%d) | LL=%d\n",
               (uint32_t)CONTROL_PERIOD_US, (uint32_t)CPU_FREQ_HZ,
               (uint32_t)TIMING_GPIO_PIN, (char)('A' + k_timing_port),
               k_timing_bit, (int)TIMING_USE_STM32_LL);
        printk("Samples: %u\n", t.count);

        print_summary("Target",        &t);
        print_summary("Critical body", &c);
#if MEASURE_ENTRY_LATENCY
        print_summary("Entry latency", &l);
#else
        printk("  Entry latency  n/a (needs TIMING_USE_STM32_LL=1 and TIM6 source)\n");
#endif
        print_summary("Period",        &p);

        printk("  Period jitter  (max-min): "); print_cycles(p.max - p.min); printk("\n");
        printk("  Period budget  (Ts)     : "); print_cycles(budget);        printk("\n");

        if ((overruns == 0U) && (worst_isr < budget))
        {
            const uint32_t slack    = budget - worst_isr;
            const uint32_t util_x10 = (uint32_t)(((uint64_t)worst_isr * 1000ULL) / budget);
            printk("Deadline: PASS\n");
            printk("  Worst case (latency+body): "); print_cycles(worst_isr); printk("\n");
            printk("  Worst-case slack         : "); print_cycles(slack);     printk("\n");
            printk("  Worst-case utilisation   : %u.%u %%\n",
                   util_x10 / 10U, util_x10 % 10U);
        }
        else
        {
            printk("Deadline: FAIL  (overruns in batch: %u, total: %u)\n",
                   overruns, overruns_total);
        }

        printk("Scope: HIGH-pulse width on pin %u = Target + E0 offset\n",
               (uint32_t)TIMING_GPIO_PIN);

#if PRINT_CSV
        /* CSV,batch,mode,N,samples,t_min,t_avg,t_max,c_min,c_avg,c_max,
         *     l_min,l_avg,l_max,p_min,p_avg,p_max,overruns,budget   (cycles) */
        printk("CSV,%u,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
               id_before, (int)TIMING_TEST_MODE, (uint32_t)TEST_SORT_N, t.count,
               t.min, t.avg, t.max, c.min, c.avg, c.max,
               l.min, l.avg, l.max, p.min, p.avg, p.max, overruns, budget);
#endif
    }

    task.suspendBackgroundMs(100U);
}

/* ============================== Setup and main ============================== */

static void setup_routine(void)
{
    const bool dwt_ok = dwt_init();

    /* Pin configured through the OwnTech API (enables the port clock and sets
     * the mode). The LL fast path then only writes BSRR/BRR of that pin. */
    spin.gpio.configurePin((uint8_t)TIMING_GPIO_PIN, OUTPUT);
    spin.gpio.resetPin((uint8_t)TIMING_GPIO_PIN);

    stats_reset(&g_target_acc);
    stats_reset(&g_critical_acc);
    stats_reset(&g_latency_acc);
    stats_reset(&g_period_acc);
    g_snapshot.batch_id = 0U;
    g_budget_cycles = (uint32_t)(((uint64_t)CPU_FREQ_HZ * CONTROL_PERIOD_US) / 1000000ULL);

    const int8_t background_task = task.createBackground(loop_background_task);
    const int8_t critical_status =
        task.createCritical(loop_critical_task, CONTROL_PERIOD_US, TIMING_IRQ_SOURCE);

    printk("\nOwnTech SPIN/TWIST timing benchmark v2\n");
    printk("  CPU clock (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC): %u Hz\n", (uint32_t)CPU_FREQ_HZ);
    printk("  Control period Ts: %u us = %u cycles\n", (uint32_t)CONTROL_PERIOD_US, g_budget_cycles);
    printk("  Timing pin: SPIN %u -> P%c%d, LL fast path: %d\n",
           (uint32_t)TIMING_GPIO_PIN, (char)('A' + k_timing_port), k_timing_bit,
           (int)TIMING_USE_STM32_LL);
    printk("  Mode: %d, N: %u, batch: %u samples, IRQ source: %s\n",
           (int)TIMING_TEST_MODE, (uint32_t)TEST_SORT_N, (uint32_t)BATCH_SAMPLES,
           TIMING_IRQ_SOURCE_TIM6 ? "TIM6" : "HRTIM");
    printk("  DWT cycle counter: %s\n", dwt_ok ? "OK" : "NOT AVAILABLE");
#if PRINT_CSV
    printk("CSV_HEADER,batch,mode,N,samples,t_min,t_avg,t_max,c_min,c_avg,c_max,"
           "l_min,l_avg,l_max,p_min,p_avg,p_max,overruns,budget_cycles\n");
#endif

    if (background_task >= 0)
    {
        task.startBackground((uint8_t)background_task);
    }
    else
    {
        printk("ERROR: could not create background task.\n");
    }

    if ((critical_status == 0) && dwt_ok)
    {
        /* false is REQUIRED: with TIM6 and no PWM, the default (true) makes
         * startCritical() return early without starting the task. */
        task.startCritical(false);
    }
    else
    {
        printk("ERROR: critical task not started (create=%d, dwt=%d).\n",
               (int)critical_status, (int)dwt_ok);
    }
}


