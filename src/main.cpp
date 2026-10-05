/*
 * Copyright (c) 2026-present LAAS-CNRS
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 2.1 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-2.1
 */

#include "timing_config.h"
#include "cvb_algorithm.h"

static volatile uint32_t result_guard;
static uint32_t input_counter;

static void prepare_cvb_input()
{
    const float32_t offset = static_cast<float32_t>(input_counter++ & 7U) * 0.01F;
    for (uint8_t i = 0; i < total_number_of_modules_arm; ++i) {
        modules_capacitor_voltages_upper_arm[i] = 75.0F - i + offset;
        modules_capacitor_voltages_lower_arm[i] = 75.0F - i + offset;
    }
    number_of_connected_submodules_upper_arm = 2.0F;
    number_of_connected_submodules_lower_arm = 3.0F;
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
    for (uint8_t i = 0; i < total_number_of_modules_arm; ++i)
        mask |= (g_u[i] << i) | (g_l[i] << (i + 5));
    result_guard = mask;
}

#include "timing_harness.h"

int main(void)
{
    setup_routine();
    return 0;
}
