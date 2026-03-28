#ifndef BUZZER_H
#define BUZZER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Buzzer configuration structure
 */
typedef struct {
    int gpio_pin;  /*!< GPIO pin number for buzzer control */
} buzzer_config_t;

/**
 * @brief Initialize buzzer on specified GPIO pin
 * @param config Pointer to buzzer configuration structure
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t buzzer_init(const buzzer_config_t *config);

/**
 * @brief Turn buzzer on
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t buzzer_on(void);

/**
 * @brief Turn buzzer off
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t buzzer_off(void);

/**
 * @brief Toggle buzzer state
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t buzzer_toggle(void);

/**
 * @brief Get current buzzer state
 * @return 1 if buzzer is on, 0 if off
 */
int buzzer_is_on(void);

#ifdef __cplusplus
}
#endif

#endif /* BUZZER_H */