/**
 * @file button.h
 * * @brief Header file for button handling with debouncing and SOS messaging.
 * * COPYRIGHT NOTICE: (c) 2022 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

#ifndef __BUTTON_H__
#define __BUTTON_H__

//--------------------------------- INCLUDES ----------------------------------

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------- MACROS -----------------------------------

/**
 * @brief Fixed size for the communication buffer.
 */
#define MAX_SEND_MSG_LEN 64

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief Initializes the button on GPIO 27, sets up the ISR service, 
 * and spawns the background task to handle debouncing and SOS signaling.
 * * @note This function configures the pin as Active High by default.
 * * @param sharedQueue Pointer to the QueueHandle_t (e.g., &xSendQueue) 
 * where 64-byte null-padded messages will be sent.
 * * @return 
 * - ESP_OK on success
 * - ESP_ERR_NO_MEM if the internal event queue or task cannot be created.
 */
esp_err_t init_button(QueueHandle_t *sharedQueue);

#ifdef __cplusplus
}
#endif

#endif // __BUTTON_H__