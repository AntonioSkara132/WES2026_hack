#include "buzzer.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "BUZZER";
static int buzzer_pin = -1;
static int buzzer_state = 0;

esp_err_t buzzer_init(const buzzer_config_t *config)
{
    ESP_LOGI(TAG, "Initializing buzzer on GPIO %d", config->gpio_pin);

    if (config == NULL) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (config->gpio_pin < 0 || config->gpio_pin > 39) {
        ESP_LOGE(TAG, "Invalid GPIO pin: %d", config->gpio_pin);
        return ESP_ERR_INVALID_ARG;
    }

    buzzer_pin = config->gpio_pin;

    // Configure GPIO for buzzer control
    gpio_config_t gpio_conf = {
        .pin_bit_mask = (1ULL << buzzer_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t ret = gpio_config(&gpio_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO pin %d", buzzer_pin);
        return ret;
    }

    // Initially turn buzzer off
    gpio_set_level(buzzer_pin, 0);
    buzzer_state = 0;

    ESP_LOGI(TAG, "Buzzer initialized successfully");
    return ESP_OK;
}

esp_err_t buzzer_on(void)
{
    if (buzzer_pin < 0) {
        ESP_LOGE(TAG, "Buzzer not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    gpio_set_level(buzzer_pin, 1);
    buzzer_state = 1;
    ESP_LOGD(TAG, "Buzzer ON");
    return ESP_OK;
}

esp_err_t buzzer_off(void)
{
    if (buzzer_pin < 0) {
        ESP_LOGE(TAG, "Buzzer not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    gpio_set_level(buzzer_pin, 0);
    buzzer_state = 0;
    ESP_LOGD(TAG, "Buzzer OFF");
    return ESP_OK;
}

esp_err_t buzzer_toggle(void)
{
    if (buzzer_pin < 0) {
        ESP_LOGE(TAG, "Buzzer not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    buzzer_state = !buzzer_state;
    gpio_set_level(buzzer_pin, buzzer_state);
    ESP_LOGD(TAG, "Buzzer toggled to %s", buzzer_state ? "ON" : "OFF");
    return ESP_OK;
}

int buzzer_is_on(void)
{
    return buzzer_state;
}