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

#pragma once
#include <stdint.h>
#include <string.h>
#include "arm_math_types.h"

// Extracted verbatim from hackathon_lille/main, ecb51f5.
static const uint8_t total_number_of_modules_arm = 5;
static uint8_t index_list[10] = {0,1,2,3,4,5,6,7,8,9};
static float32_t number_of_connected_submodules_upper_arm;
static float32_t number_of_connected_submodules_lower_arm;
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

