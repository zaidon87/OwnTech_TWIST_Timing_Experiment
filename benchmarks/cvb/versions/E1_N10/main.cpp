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
#include "cvb_cases.h"
#include "timing_harness.h"

int main(void)
{
    const bool ok = cvb_selftest();
    printk("CVB_SELFTEST,%s,checks=%u,failures=%u\n",
           ok ? "PASS" : "FAIL", cvb_selftest_checks, cvb_selftest_failures);
    if (!ok) {
        printk("ERROR: timing not started because CVB correctness failed.\n");
        return 0;
    }
    setup_routine();
    return 0;
}
