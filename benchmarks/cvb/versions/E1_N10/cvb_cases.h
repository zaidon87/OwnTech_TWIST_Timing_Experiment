#pragma once
#include "cvb_algorithm.h"

// Campaign: 6 patterns x 5 current pairs x (N+1) insertion-count pairs.
static constexpr uint32_t CVB_N = CVB_MODULES_PER_ARM;
static constexpr uint32_t CVB_COUNT_OPTIONS = CVB_N + 1U;
static constexpr uint32_t CVB_CASES_PER_PATTERN = 5U * CVB_COUNT_OPTIONS;
static constexpr uint32_t CVB_CASE_COUNT = 6U * CVB_CASES_PER_PATTERN;
static constexpr uint32_t CVB_SELFTEST_EXPECTED = 369U * 9U * CVB_COUNT_OPTIONS * CVB_COUNT_OPTIONS;
static constexpr uint32_t CVB_BATCHES_PER_CASE = 3;
static const char *const cvb_pattern_names[] = {
    "ascending", "descending", "equal", "ties", "alternating", "random"
};
static constexpr int8_t cvb_current_pairs[5][2] = {
    {1, 1}, {-1, -1}, {1, -1}, {-1, 1}, {0, 0}
};
static uint32_t cvb_selftest_checks;
static uint32_t cvb_selftest_failures;
static uint32_t cvb_active_case;
static uint32_t cvb_sample_number;
static volatile uint32_t cvb_result_guard;

static void cvb_make_pattern(uint32_t pattern, float32_t *values)
{
    static constexpr float32_t patterns[5][5] = {
        {70, 71, 72, 73, 74}, {74, 73, 72, 71, 70}, {72, 72, 72, 72, 72},
        {71, 70, 71, 70, 72}, {70, 74, 71, 73, 72}
    };
    uint32_t seed = 0x43564231U;
    for (uint8_t i = 0; i < CVB_N; ++i) {
        seed = seed * 1664525U + 1013904223U;
        if constexpr (CVB_N == 5) {
            values[i] = pattern < 5 ? patterns[pattern][i]
                                   : 70.0F + static_cast<float32_t>((seed >> 16) % 501U) * 0.01F;
        } else {
            switch (pattern) {
            case 0: values[i] = 70.0F + i; break;
            case 1: values[i] = 70.0F + (CVB_N - 1U - i); break;
            case 2: values[i] = 72.0F; break;
            case 3: values[i] = 70.0F + (i % 3U); break;
            case 4: values[i] = 70.0F + ((i & 1U) ? CVB_N - 1U - i / 2U : i / 2U); break;
            default: values[i] = 70.0F + static_cast<float32_t>((seed >> 16) % 1001U) * 0.01F;
            }
        }
    }
}

static void prepare_cvb_input()
{
    cvb_active_case = (cvb_sample_number / (BATCH_SAMPLES * CVB_BATCHES_PER_CASE)) % CVB_CASE_COUNT;
    const uint32_t pattern = cvb_active_case / CVB_CASES_PER_PATTERN;
    const uint32_t currents = (cvb_active_case / CVB_COUNT_OPTIONS) % 5U;
    const uint32_t count = cvb_active_case % CVB_COUNT_OPTIONS;
    float32_t values[CVB_N];
    cvb_make_pattern(pattern, values);
    // Vary the offset without changing ordering or ties; inputs remain runtime data.
    const float32_t offset = static_cast<float32_t>(cvb_sample_number++ & 7U) * 0.01F;
    for (uint8_t i = 0; i < CVB_N; ++i) {
        modules_capacitor_voltages_upper_arm[i] = values[i] + offset;
        modules_capacitor_voltages_lower_arm[i] = values[i] + offset;
    }
    i_upper_arm = cvb_current_pairs[currents][0];
    i_lower_arm = cvb_current_pairs[currents][1];
    number_of_connected_submodules_upper_arm = static_cast<float32_t>(count);
    number_of_connected_submodules_lower_arm = static_cast<float32_t>(CVB_N - count);
}

static void cvb_execute()
{
#if CVB_SCOPE != 2
    sorting_upper_arm();
#endif
#if CVB_SCOPE != 1
    sorting_lower_arm();
#endif
}

static void consume_cvb_result()
{
    uint32_t mask = 0;
    for (uint8_t i = 0; i < CVB_N; ++i)
        mask |= (g_u[i] << i) | (g_l[i] << (i + CVB_N));
    cvb_result_guard = mask;
}

// Independent rank reference: ascending stable ties, reversed for negative current.
// The production bubble sort is stable; reverse traversal favors higher IDs on ties.
static bool cvb_check_arm(const float32_t *input, float32_t current,
                          uint8_t count, const uint8_t *gates,
                          const float32_t *sorted, const uint8_t *indexes)
{
    uint32_t seen = 0;
    for (uint8_t i = 0; i < CVB_N; ++i) {
        uint8_t rank = 0;
        for (uint8_t j = 0; j < CVB_N; ++j) {
            const bool ahead = current >= 0 ? input[j] < input[i] : input[j] > input[i];
            const bool tie = input[j] == input[i] && (current >= 0 ? j < i : j > i);
            if (ahead || tie) ++rank;
        }
        if (gates[i] != static_cast<uint8_t>(rank < count)) return false;
        if (indexes[i] >= CVB_N || (seen & (1U << indexes[i]))) return false;
        seen |= 1U << indexes[i];
        if (sorted[i] != input[indexes[i]]) return false;
        if (i && (sorted[i-1] > sorted[i] ||
                  (sorted[i-1] == sorted[i] && indexes[i-1] > indexes[i]))) return false;
    }
    return seen == (1U << CVB_N) - 1U;
}

static void cvb_test_vector(const float32_t *upper)
{
    float32_t lower[CVB_N];
    for (uint8_t i = 0; i < CVB_N; ++i) lower[i] = upper[CVB_N-1U-i] + 1.0F;
    for (int8_t iu = -1; iu <= 1; ++iu)
        for (int8_t il = -1; il <= 1; ++il)
            for (uint8_t nu = 0; nu <= CVB_N; ++nu)
                for (uint8_t nl = 0; nl <= CVB_N; ++nl) {
                    memcpy(modules_capacitor_voltages_upper_arm, upper, sizeof(lower));
                    memcpy(modules_capacitor_voltages_lower_arm, lower, sizeof(lower));
                    i_upper_arm = iu;
                    i_lower_arm = il;
                    number_of_connected_submodules_upper_arm = nu;
                    number_of_connected_submodules_lower_arm = nl;
                    sorting_upper_arm();
                    sorting_lower_arm();
                    ++cvb_selftest_checks;
                    if (!cvb_check_arm(upper, iu, nu, g_u, modules_capacitor_voltages_upper_arm, modules_indexes_upper_arm) ||
                        !cvb_check_arm(lower, il, nl, g_l, modules_capacitor_voltages_lower_arm, modules_indexes_lower_arm))
                        ++cvb_selftest_failures;
                }
}

static bool cvb_selftest()
{
    cvb_selftest_checks = 0;
    cvb_selftest_failures = 0;
    float32_t values[CVB_N];
    for (uint32_t pattern = 0; pattern < 6; ++pattern) {
        cvb_make_pattern(pattern, values);
        cvb_test_vector(values);
    }
    // N=5: exhaustive ternary vectors. N=10: 243 bounded tied vectors,
    // with a mirrored/shifted second half to exercise all ten indices.
    for (uint32_t arrangement = 0; arrangement < 243; ++arrangement) {
        uint32_t code = arrangement;
        for (uint8_t i = 0; i < CVB_N; ++i) {
            if (i < 5) {
                values[i] = 70.0F + static_cast<float32_t>(code % 3);
                code /= 3;
            } else {
                values[i] = 70.0F + static_cast<float32_t>((static_cast<uint32_t>(values[CVB_N-1U-i] - 70.0F) + arrangement % 3U) % 3U);
            }
        }
        cvb_test_vector(values);
    }
    // N=5: all 5! permutations. N=10: 120 seeded shuffled permutations,
    // bounded deliberately (not exhaustive 10! enumeration).
    uint32_t shuffle_seed = 0x43564210U;
    for (uint32_t permutation = 0; permutation < 120; ++permutation) {
        uint8_t remaining[CVB_N];
        for (uint8_t i = 0; i < CVB_N; ++i) remaining[i] = i;
        if constexpr (CVB_N == 5) {
            uint32_t code = permutation;
            for (uint8_t i = 0; i < CVB_N; ++i) {
                uint8_t n = CVB_N-i;
                uint8_t index = code % n;
                code /= n;
                values[i] = 70.0F + remaining[index];
                for (uint8_t j = index; j+1 < n; ++j) remaining[j] = remaining[j+1];
            }
        } else {
            for (uint8_t n = CVB_N; n > 1; --n) {
                shuffle_seed = shuffle_seed * 1664525U + 1013904223U;
                const uint8_t j = (shuffle_seed >> 16) % n;
                const uint8_t temp = remaining[n-1];
                remaining[n-1] = remaining[j];
                remaining[j] = temp;
            }
            for (uint8_t i = 0; i < CVB_N; ++i) values[i] = 70.0F + remaining[i];
        }
        cvb_test_vector(values);
    }
    memset(g_u, 0, sizeof(g_u));
    memset(g_l, 0, sizeof(g_l));
    return cvb_selftest_checks == CVB_SELFTEST_EXPECTED && cvb_selftest_failures == 0;
}
