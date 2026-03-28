#include "button.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "BUTTON";
static int button_pin = -1;
static int button_active_low = 1;
static int button_debounce_ms = 20;
static button_callback_t button_callback = NULL;
static TaskHandle_t debounce_task = NULL;
static QueueHandle_t button_evt_queue = NULL;
static int last_debounced_state = -1;

typedef struct {
    int timestamp;
} button_event_t;

static void IRAM_ATTR button_isr_handler(void *arg)
{
    button_event_t evt = {0};
    xQueueSendFromISR(button_evt_queue, &evt, NULL);
}

static void button_debounce_task_func(void *arg)
{
    button_event_t evt;

    while (1) {
        if (xQueueReceive(button_evt_queue, &evt, portMAX_DELAY)) {
            // Wait for debounce period
            vTaskDelay(pdMS_TO_TICKS(button_debounce_ms));

            // Read state after debounce
            int raw_state = gpio_get_level(button_pin);
            int logical_state = button_active_low ? !raw_state : raw_state;

            // Only trigger callback if state actually changed (debounced)
            if (logical_state != last_debounced_state) {
                last_debounced_state = logical_state;
                ESP_LOGD(TAG, "Button state changed: %s", logical_state ? "PRESSED" : "RELEASED");

                if (button_callback != NULL) {
                    button_callback(logical_state);
                }
            }
        }
    }
}

esp_err_t button_init(const button_config_t *config)
{
    ESP_LOGI(TAG, "Initializing button on GPIO %d (interrupt mode)", config->gpio_pin);

    if (config == NULL) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (config->gpio_pin < 0 || config->gpio_pin > 39) {
        ESP_LOGE(TAG, "Invalid GPIO pin: %d", config->gpio_pin);
        return ESP_ERR_INVALID_ARG;
    }

    button_pin = config->gpio_pin;
    button_active_low = config->active_low;
    button_debounce_ms = config->debounce_ms > 0 ? config->debounce_ms : 20;
    button_callback = config->callback;

    // Create queue for button events
    button_evt_queue = xQueueCreate(10, sizeof(button_event_t));
    if (button_evt_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create button event queue");
        return ESP_ERR_NO_MEM;
    }

    // Configure GPIO for button input
    gpio_config_t gpio_conf = {
        .pin_bit_mask = (1ULL << button_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = button_active_low ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = button_active_low ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE  // Trigger on both rising and falling edges
    };

    esp_err_t ret = gpio_config(&gpio_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO pin %d", button_pin);
        vQueueDelete(button_evt_queue);
        return ret;
    }

    // Install ISR service
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {  // ESP_ERR_INVALID_STATE means service already installed
        ESP_LOGE(TAG, "Failed to install GPIO ISR service");
        vQueueDelete(button_evt_queue);
        return ret;
    }

    // Attach ISR handler
    ret = gpio_isr_handler_add(button_pin, button_isr_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to attach ISR handler");
        vQueueDelete(button_evt_queue);
        return ret;
    }

    // Create debounce handling task
    if (xTaskCreate(button_debounce_task_func, "button_debounce", 2048, NULL, 5, &debounce_task) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button debounce task");
        gpio_isr_handler_remove(button_pin);
        vQueueDelete(button_evt_queue);
        return ESP_ERR_NO_MEM;
    }

    // Initialize last_debounced_state
    int raw_state = gpio_get_level(button_pin);
    last_debounced_state = button_active_low ? !raw_state : raw_state;

    ESP_LOGI(TAG, "Button initialized successfully (active %s, interrupt mode)", button_active_low ? "LOW" : "HIGH");
    return ESP_OK;
}

int button_is_pressed(void)
{
    if (button_pin < 0) {
        ESP_LOGE(TAG, "Button not initialized");
        return -1;
    }

    int raw_state = gpio_get_level(button_pin);
    int logical_state = button_active_low ? !raw_state : raw_state;
    return logical_state;
}

int button_get_raw_state(void)
{
    if (button_pin < 0) {
        ESP_LOGE(TAG, "Button not initialized");
        return -1;
    }

    return gpio_get_level(button_pin);
}

esp_err_t button_deinit(void)
{
    if (button_pin < 0) {
        ESP_LOGE(TAG, "Button not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Remove ISR handler
    esp_err_t ret = gpio_isr_handler_remove(button_pin);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to remove ISR handler");
    }

    // Delete debounce task
    if (debounce_task != NULL) {
        vTaskDelete(debounce_task);
        debounce_task = NULL;
    }

    // Delete event queue
    if (button_evt_queue != NULL) {
        vQueueDelete(button_evt_queue);
        button_evt_queue = NULL;
    }

    button_pin = -1;
    button_callback = NULL;
    last_debounced_state = -1;

    ESP_LOGI(TAG, "Button deinitialized");
    return ESP_OK;
}