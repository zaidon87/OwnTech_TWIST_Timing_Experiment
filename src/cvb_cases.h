#pragma once
#include "cvb_algorithm.h"

// Campaign: 6 patterns x 5 current pairs x 6 insertion-count pairs.
static constexpr uint32_t CVB_CASE_COUNT = 180;
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
    for (uint8_t i = 0; i < 5; ++i) {
        seed = seed * 1664525U + 1013904223U;
        values[i] = pattern < 5 ? patterns[pattern][i]
                               : 70.0F + static_cast<float32_t>((seed >> 16) % 501U) * 0.01F;
    }
}

static void prepare_cvb_input()
{
    cvb_active_case = (cvb_sample_number / (BATCH_SAMPLES * CVB_BATCHES_PER_CASE)) % CVB_CASE_COUNT;
    const uint32_t pattern = cvb_active_case / 30U;
    const uint32_t currents = (cvb_active_case / 6U) % 5U;
    const uint32_t count = cvb_active_case % 6U;
    float32_t values[5];
    cvb_make_pattern(pattern, values);
    // Vary the offset without changing ordering or ties; inputs remain runtime data.
    const float32_t offset = static_cast<float32_t>(cvb_sample_number++ & 7U) * 0.01F;
    for (uint8_t i = 0; i < 5; ++i) {
        modules_capacitor_voltages_upper_arm[i] = values[i] + offset;
        modules_capacitor_voltages_lower_arm[i] = values[i] + offset;
    }
    i_upper_arm = cvb_current_pairs[currents][0];
    i_lower_arm = cvb_current_pairs[currents][1];
    number_of_connected_submodules_upper_arm = static_cast<float32_t>(count);
    number_of_connected_submodules_lower_arm = static_cast<float32_t>(5U - count);
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
    for (uint8_t i = 0; i < 5; ++i)
        mask |= (g_u[i] << i) | (g_l[i] << (i + 5));
    cvb_result_guard = mask;
}

// Independent rank reference: ascending stable ties, reversed for negative current.
// The production bubble sort is stable; reverse traversal favors higher IDs on ties.
static bool cvb_check_arm(const float32_t *input, float32_t current,
                          uint8_t count, const uint8_t *gates,
                          const float32_t *sorted, const uint8_t *indexes)
{
    uint8_t seen = 0;
    for (uint8_t i = 0; i < 5; ++i) {
        uint8_t rank = 0;
        for (uint8_t j = 0; j < 5; ++j) {
            const bool ahead = current >= 0 ? input[j] < input[i] : input[j] > input[i];
            const bool tie = input[j] == input[i] && (current >= 0 ? j < i : j > i);
            if (ahead || tie) ++rank;
        }
        if (gates[i] != static_cast<uint8_t>(rank < count)) return false;
        if (indexes[i] >= 5 || (seen & (1U << indexes[i]))) return false;
        seen |= 1U << indexes[i];
        if (sorted[i] != input[indexes[i]]) return false;
        if (i && (sorted[i-1] > sorted[i] ||
                  (sorted[i-1] == sorted[i] && indexes[i-1] > indexes[i]))) return false;
    }
    return seen == 31;
}

static void cvb_test_vector(const float32_t *upper)
{
    float32_t lower[5];
    for (uint8_t i = 0; i < 5; ++i) lower[i] = upper[4-i] + 1.0F;
    for (int8_t iu = -1; iu <= 1; ++iu)
        for (int8_t il = -1; il <= 1; ++il)
            for (uint8_t nu = 0; nu <= 5; ++nu)
                for (uint8_t nl = 0; nl <= 5; ++nl) {
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
    float32_t values[5];
    for (uint32_t pattern = 0; pattern < 6; ++pattern) {
        cvb_make_pattern(pattern, values);
        cvb_test_vector(values);
    }
    // Exhaust all 3^5 tied voltage arrangements.
    for (uint32_t arrangement = 0; arrangement < 243; ++arrangement) {
        uint32_t code = arrangement;
        for (uint8_t i = 0; i < 5; ++i) {
            values[i] = 70.0F + static_cast<float32_t>(code % 3);
            code /= 3;
        }
        cvb_test_vector(values);
    }
    // Exhaust all 5! distinct orderings using a factoradic permutation.
    for (uint32_t permutation = 0; permutation < 120; ++permutation) {
        uint8_t remaining[5] = {0,1,2,3,4};
        uint32_t code = permutation;
        for (uint8_t i = 0; i < 5; ++i) {
            uint8_t n = 5-i;
            uint8_t index = code % n;
            code /= n;
            values[i] = 70.0F + remaining[index];
            for (uint8_t j = index; j+1 < n; ++j) remaining[j] = remaining[j+1];
        }
        cvb_test_vector(values);
    }
    memset(g_u, 0, sizeof(g_u));
    memset(g_l, 0, sizeof(g_l));
    return cvb_selftest_checks == 119556U && cvb_selftest_failures == 0;
}
