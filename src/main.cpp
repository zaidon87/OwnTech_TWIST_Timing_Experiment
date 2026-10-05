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

#include "cvb_algorithm.h"

// Standalone CVB baseline: no power, sensors or communication initialized.
static volatile uint32_t result_guard;
int main(void)
{
    for (uint8_t i = 0; i < total_number_of_modules_arm; ++i) {
        modules_capacitor_voltages_upper_arm[i] = 75.0F - i;
        modules_capacitor_voltages_lower_arm[i] = 70.0F + i;
    }
    number_of_connected_submodules_upper_arm = 2.0F;
    number_of_connected_submodules_lower_arm = 3.0F;
    sorting_upper_arm();
    sorting_lower_arm();
    uint32_t mask = 0;
    for (uint8_t i = 0; i < total_number_of_modules_arm; ++i)
        mask |= (g_u[i] << i) | (g_l[i] << (i + 5));
    result_guard = mask;
    return 0;
}
