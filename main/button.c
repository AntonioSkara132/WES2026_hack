#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define MAX_SEND_MSG_LEN 64
static const char *TAG = "BUTTON_APP";

static int g_button_pin = 33;
static int g_active_low = 1; // Set to 1 if using internal pull-up
static QueueHandle_t button_evt_queue = NULL;
static QueueHandle_t *pSendQueue = NULL;

static void IRAM_ATTR button_isr_handler(void *arg) {
    int pin = (int)arg;
    xQueueSendFromISR(button_evt_queue, &pin, NULL);
}

void button_to_queue_task(void *pvParameters) {
    pSendQueue = (QueueHandle_t *)pvParameters;
    int pin_num;
    int last_state = -1;

    while (1) {
        if (xQueueReceive(button_evt_queue, &pin_num, portMAX_DELAY)) {
            vTaskDelay(pdMS_TO_TICKS(20)); // Debounce

            int raw_state = gpio_get_level(g_button_pin);
            int logical_state = g_active_low ? !raw_state : raw_state;

            if (logical_state != last_state) {
                last_state = logical_state;

                // Create 64-byte buffer initialized to all zeros
                char tx_buf[MAX_SEND_MSG_LEN] = {0};

                if (logical_state) {
                    ESP_LOGI(TAG, "SOS Pressed!");
                    snprintf(tx_buf, MAX_SEND_MSG_LEN, "SOS");
                } else {
                    ESP_LOGI(TAG, "Button Released.");
                    //snprintf(tx_buf, MAX_SEND_MSG_LEN, "RELEASED\n");
                }

                // Send exactly 64 bytes to the communication queue
                if (xQueueSend(*pSendQueue, tx_buf, pdMS_TO_TICKS(100)) != pdTRUE) {
                    ESP_LOGW(TAG, "Send queue full!");
                }
            }
        }
    }
}

esp_err_t init_button(QueueHandle_t *sharedQueue) {
    button_evt_queue = xQueueCreate(10, sizeof(int));
    if (!button_evt_queue) return ESP_ERR_NO_MEM;

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << g_button_pin),
        .pull_down_en = g_active_low ? 0 : 1,
        .pull_up_en = g_active_low ? 1 : 0,
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(g_button_pin, button_isr_handler, (void*) g_button_pin);

    xTaskCreate(button_to_queue_task, "btn_task", 3072, sharedQueue, 6, NULL);
    return ESP_OK;
}