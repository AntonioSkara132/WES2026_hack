#ifndef BUTTON_H
#define BUTTON_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Button event callback function type
 * @param pressed 1 if button is pressed, 0 if released
 */
typedef void (*button_callback_t)(int pressed);

/**
 * @brief Button configuration structure
 */
typedef struct {
    int gpio_pin;                    /*!< GPIO pin number for button */
    int active_low;                  /*!< 1 if button is active low (pull-up), 0 if active high (pull-down) */
    int debounce_ms;                 /*!< Debounce time in milliseconds */
    button_callback_t callback;      /*!< Optional callback function for button events (NULL to disable) */
} button_config_t;

/**
 * @brief Initialize button on specified GPIO pin
 * @param config Pointer to button configuration structure
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t button_init(const button_config_t *config);

/**
 * @brief Read button state (debounced)
 * @return 1 if button is pressed, 0 if not pressed; -1 on error
 */
int button_is_pressed(void);

/**
 * @brief Get raw button GPIO state
 * @return GPIO level (0 or 1)
 */
int button_get_raw_state(void);

/**
 * @brief Deinitialize button
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t button_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_H */