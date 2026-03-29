/**
 * @file buzzer.h
 * * @brief Header file for controlling a simple digital buzzer on ESP32.
 * * COPYRIGHT NOTICE: (c) 2022 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

#ifndef __BUZZER_H__
#define __BUZZER_H__

//--------------------------------- INCLUDES ----------------------------------

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------- MACROS -----------------------------------

/**
 * @brief GPIO Pin assigned to the Buzzer.
 */
#define BUZZER_GPIO     GPIO_NUM_2

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief Initialize the buzzer GPIO as a digital output and set it to low.
 */
void buzzer_init(void);

/**
 * @brief Set the buzzer state to ON or OFF.
 * * @param state true to turn buzzer ON, false to turn it OFF.
 */
void buzzer_set_state(bool state);

/**
 * @brief Beep the buzzer for a specific duration.
 * * @note This function is blocking for the calling task during the duration.
 * * @param ms Duration of the beep in milliseconds.
 */
void buzzer_beep(uint32_t ms);
void buzzer_test_task(void *pvParameters);
void buzzer_test(void);

#ifdef __cplusplus
}
#endif

#endif // __BUZZER_H__