#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "buzzer.h"
#include "esp_log.h"

#define BUZZER_GPIO     GPIO_NUM_2

/**
 * @brief Initialize the buzzer GPIO as a digital output.
 */
void buzzer_init(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << BUZZER_GPIO),
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);
    // Ensure buzzer is initially off
    gpio_set_level(BUZZER_GPIO, 0);
}

/**
 * @brief Set the buzzer state.
 * @param state true = on, false = off
 */
void buzzer_set_state(bool state)
{
    gpio_set_level(BUZZER_GPIO, state ? 1 : 0);
}

/**
 * @brief Beep for a given duration (blocking).
 * @param ms Duration in milliseconds
 */


void buzzer_beep(uint32_t ms)
{
    buzzer_set_state(true);
    vTaskDelay(pdMS_TO_TICKS(ms));
    buzzer_set_state(false);
}

static const char *T_TAG = "BUZZER_TEST";

void buzzer_test_task(void *pvParameters)
{
    ESP_LOGI(T_TAG, "Starting Buzzer Test Task...");
    
    // Ensure buzzer is initialized
    buzzer_init();

    while (1) {
        // Pattern 1: Short "chirp"
        ESP_LOGD(T_TAG, "Pattern: Short Chirp");
        buzzer_beep(100); 
        vTaskDelay(pdMS_TO_TICKS(200));

        // Pattern 2: Two quick beeps
        ESP_LOGD(T_TAG, "Pattern: Double Beep");
        buzzer_beep(500);
        vTaskDelay(pdMS_TO_TICKS(500));
        buzzer_beep(500);
        
        // Wait 2 seconds before repeating the test cycle
        ESP_LOGI(T_TAG, "Cycle complete. Waiting 2 seconds...");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void buzzer_test(void)
{
    xTaskCreate(buzzer_test_task, "buzzer_test_task", 2048, NULL, 5, NULL);
}
