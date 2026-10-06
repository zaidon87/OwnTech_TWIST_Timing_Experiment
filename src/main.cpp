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

// Standalone lead benchmark derived from the Lille reference (ecb51f5).
// Flash this image on the lead. No UID detection, RS485, sync, ADC or PWM.
#include "SpinAPI.h"
#include "TaskAPI.h"
#include "arm_math_types.h"
#include <stdint.h>
#include <string.h>
#include <stm32_ll_gpio.h>
#include <zephyr/sys/printk.h>

// Number of modules sorted in EACH arm (two arms per PC7 pulse).
// Suggested campaign: 1, 5, 10, 15, 20, 25, 30. Rebuild/flash for each case.
#ifndef SORT_MODULES_PER_ARM
#define SORT_MODULES_PER_ARM 5
#endif
static_assert(SORT_MODULES_PER_ARM >= 1 && SORT_MODULES_PER_ARM <= 30,
              "Select 1..30 modules per arm");

// Repeat the same input every 5 ms, leaving headroom for the 30-module case.
#ifndef SORT_PERIOD_US
#define SORT_PERIOD_US 500
#endif
static_assert(SORT_PERIOD_US >= 1 && SORT_PERIOD_US <= 6553,
              "TIM6 requires a period in 1..6553 us");

// Lille ecb51f5 routines preserved below; only array sizing is generalized.
static constexpr uint8_t total_number_of_modules_arm = SORT_MODULES_PER_ARM;
static uint8_t index_list[total_number_of_modules_arm];
static float32_t number_of_connected_submodules_upper_arm = total_number_of_modules_arm;
static float32_t number_of_connected_submodules_lower_arm = total_number_of_modules_arm;
static float32_t modules_capacitor_voltages_upper_arm[total_number_of_modules_arm];
static uint8_t modules_indexes_upper_arm[total_number_of_modules_arm];
static float32_t modules_capacitor_voltages_lower_arm[total_number_of_modules_arm];
static uint8_t modules_indexes_lower_arm[total_number_of_modules_arm];
static float32_t i_upper_arm = 1.0F;
static float32_t i_lower_arm = -1.0F;
static uint8_t g_u[total_number_of_modules_arm];
static uint8_t g_l[total_number_of_modules_arm];

void sorting_upper_arm()
{
    /* Reset upper modules indexes every time the function is used */
    memcpy(modules_indexes_upper_arm, index_list, total_number_of_modules_arm);
    
    /* Sorts upper modules indexes according to capacitor voltage in ascending order (lower to higher voltage) */
    uint8_t counter_loops_sorting = 0;
    while(counter_loops_sorting < total_number_of_modules_arm + 1){ 
            /* Bubble sorting technique - simple */
            for(uint8_t counter = 0; counter < total_number_of_modules_arm-1; counter++)
            {
                if(modules_capacitor_voltages_upper_arm[counter] > modules_capacitor_voltages_upper_arm[counter + 1])
                {
                    float32_t temp = modules_capacitor_voltages_upper_arm[counter];
                    modules_capacitor_voltages_upper_arm[counter] = modules_capacitor_voltages_upper_arm[counter + 1];
                    modules_capacitor_voltages_upper_arm[counter + 1] = temp;
                    float32_t temp2 = modules_indexes_upper_arm[counter];
                    modules_indexes_upper_arm[counter] = modules_indexes_upper_arm[counter + 1];
                    modules_indexes_upper_arm[counter + 1] = temp2;
                }
            }

            counter_loops_sorting++;
        }

    /* Choses the modules to connect to the upper arm according to capacitor voltages and arm current */
    for(uint8_t counter = 0; counter < total_number_of_modules_arm; counter++)
        {
            /* Positive arm current */
            // Connect modules with smallest capacitor voltages
            // Disconnect modules with highest capacitor voltages

            if(i_upper_arm>=0)
            {
                uint8_t index_smallest_voltage_capacitor_upper_arm = modules_indexes_upper_arm[counter];
                if(counter < number_of_connected_submodules_upper_arm)
                {
                    g_u[index_smallest_voltage_capacitor_upper_arm] = 1;
                }
                else{
                    g_u[index_smallest_voltage_capacitor_upper_arm] = 0;
                }
            }

            /* Negative arm current */
            // Connect modules with highest capacitor voltages
            // Disconnect modules with smallest capacitor voltages
            if(i_upper_arm<0)
            {
                uint8_t higher_index = total_number_of_modules_arm-1-counter;
                uint8_t index_highest_voltage_capacitor_upper_arm = modules_indexes_upper_arm[higher_index];
                if(counter < number_of_connected_submodules_upper_arm)
                {
                    g_u[index_highest_voltage_capacitor_upper_arm] = 1;
                }
                else{
                    g_u[index_highest_voltage_capacitor_upper_arm] = 0;
                }
            }   
        }

}

/**
 * @brief Capacitor Voltage Balancing (CVB) algorithm - Determine which modules connect/disconnect on upper arm.
 *
 * @param ...
 * @return Gate signals for upper arm.
 */
void sorting_lower_arm()
{
    /* Reset lower modules indexes every time the function is used */
    memcpy(modules_indexes_lower_arm, index_list, total_number_of_modules_arm);
    
    /* Sorts lower modules indexes according to capacitor voltage in ascending order (lower to higher voltage) */
    uint8_t counter_loops_sorting = 0;
    while(counter_loops_sorting < total_number_of_modules_arm + 1){ 
            /* Bubble sorting technique - simple */
            for(uint8_t counter = 0; counter < total_number_of_modules_arm-1; counter++)
            {
                if(modules_capacitor_voltages_lower_arm[counter] > modules_capacitor_voltages_lower_arm[counter + 1])
                {
                    float32_t temp = modules_capacitor_voltages_lower_arm[counter];
                    modules_capacitor_voltages_lower_arm[counter] = modules_capacitor_voltages_lower_arm[counter + 1];
                    modules_capacitor_voltages_lower_arm[counter + 1] = temp;
                    float32_t temp2 = modules_indexes_lower_arm[counter];
                    modules_indexes_lower_arm[counter] = modules_indexes_lower_arm[counter + 1];
                    modules_indexes_lower_arm[counter + 1] = temp2;
                }
            }

            counter_loops_sorting++;
        }

    /* Choses the modules to connect to the lower arm according to capacitor voltages and arm current */
    for(uint8_t counter = 0; counter < total_number_of_modules_arm; counter++)
        {
            /* Positive arm current */
            // Connect modules with smallest capacitor voltages
            // Disconnect modules with highest capacitor voltages

            if(i_lower_arm>=0)
            {
                uint8_t index_smallest_voltage_capacitor_lower_arm = modules_indexes_lower_arm[counter];
                if(counter < number_of_connected_submodules_lower_arm)
                {
                    g_l[index_smallest_voltage_capacitor_lower_arm] = 1;
                }
                else{
                    g_l[index_smallest_voltage_capacitor_lower_arm] = 0;
                }
            }

            /* Negative arm current */
            // Connect modules with highest capacitor voltages
            // Disconnect modules with smallest capacitor voltages
            if(i_lower_arm<0)
            {
                uint8_t higher_index = total_number_of_modules_arm-1-counter;
                uint8_t index_highest_voltage_capacitor_lower_arm = modules_indexes_lower_arm[higher_index];
                if(counter < number_of_connected_submodules_lower_arm)
                {
                    g_l[index_highest_voltage_capacitor_lower_arm] = 1;
                }
                else{
                    g_l[index_highest_voltage_capacitor_lower_arm] = 0;
                }
            }   
        }

}


// Generate reproducible inputs once at startup; keep unsorted copies for reuse.
static uint32_t random_state = 0x43564231U;
static volatile uint32_t sorting_result_guard;
static float32_t initial_voltages_upper_arm[total_number_of_modules_arm];
static float32_t initial_voltages_lower_arm[total_number_of_modules_arm];

static float32_t random_voltage()
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return 70.0F + static_cast<float32_t>(random_state & 0xFFFFU) * (10.0F / 65535.0F);
}

static void loop_critical_task()
{
    // Sorting mutates the working arrays. Restore identical unsorted inputs
    // before raising PC7 so every measurement executes the same sorting case.
    memcpy(modules_capacitor_voltages_upper_arm, initial_voltages_upper_arm,
           sizeof(initial_voltages_upper_arm));
    memcpy(modules_capacitor_voltages_lower_arm, initial_voltages_lower_arm,
           sizeof(initial_voltages_lower_arm));

    // // Memory barriers keep input preparation and result consumption outside
    // // the pulse. Direct GPIO writes minimize marker overhead.
    // asm volatile("" ::: "memory");
    LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_7);
    // asm volatile("" ::: "memory");
    sorting_upper_arm();
    sorting_lower_arm();
    // asm volatile("" ::: "memory");
    LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_7);
    // asm volatile("" ::: "memory");

    // Consume indexes and gates after timing to retain both sorting results.
    uint32_t result = 0;
    for (uint8_t i = 0; i < total_number_of_modules_arm; ++i) {
        result = result * 33U + modules_indexes_upper_arm[i];
        result = result * 33U + modules_indexes_lower_arm[i];
        result = result * 33U + g_u[i] + 2U * g_l[i];
    }
    sorting_result_guard = result;
}

int main(void)
{
    for (uint8_t i = 0; i < total_number_of_modules_arm; ++i) {
        index_list[i] = i;
        initial_voltages_upper_arm[i] = random_voltage();
        initial_voltages_lower_arm[i] = random_voltage();
    }
    spin.gpio.configurePin(PC7, OUTPUT);
    spin.gpio.resetPin(PC7);
    // TIM6 runs independently of the power stage; false avoids PWM sync.
    const int8_t status = task.createCritical(loop_critical_task,
                                              SORT_PERIOD_US, source_tim6);
    if (status != 0) {
        printk("Sorting benchmark: critical task creation failed (%d)\n", status);
        return 0;
    }
    task.startCritical(false);
    return 0;
}
