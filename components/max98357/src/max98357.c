#include "max98357.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

static const char *TAG = "MAX98357";
static i2s_port_t i2s_port = I2S_NUM_0;
static int sd_pin = -1;

esp_err_t max98357_init(const max98357_config_t *config)
{
    ESP_LOGI(TAG, "Initializing MAX98357 amplifier...");

    if (config == NULL) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    sd_pin = config->sd_pin;

    // Configure GPIO for SD_MODE (speaker enable/disable)
    gpio_config_t gpio_conf = {
        .pin_bit_mask = (1ULL << sd_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t ret = gpio_config(&gpio_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO pin %d", sd_pin);
        return ret;
    }

    // Initially disable amplifier
    gpio_set_level(sd_pin, 0);

    // Configure I2S
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = config->sample_rate,
        .bits_per_sample = config->bits_per_sample,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = 0,
        .dma_buf_count = config->dma_buf_count,
        .dma_buf_len = config->dma_buf_len,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    ret = i2s_driver_install(i2s_port, &i2s_config, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2S driver: %s", esp_err_to_name(ret));
        return ret;
    }

    i2s_pin_config_t pin_config = {
        .bck_io_num = config->bck_pin,
        .ws_io_num = config->ws_pin,
        .data_out_num = config->dout_pin,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    ret = i2s_set_pin(i2s_port, &pin_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pins: %s", esp_err_to_name(ret));
        i2s_driver_uninstall(i2s_port);
        return ret;
    }

    ESP_LOGI(TAG, "MAX98357 initialized successfully");
    ESP_LOGI(TAG, "Sample rate: %d Hz, Bits per sample: %d, SD_MODE pin: %d",
             config->sample_rate, config->bits_per_sample, sd_pin);

    return ESP_OK;
}

esp_err_t max98357_amp_enable(void)
{
    if (sd_pin < 0) {
        ESP_LOGE(TAG, "MAX98357 not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    gpio_set_level(sd_pin, 1);
    ESP_LOGI(TAG, "Amplifier enabled");
    return ESP_OK;
}

esp_err_t max98357_amp_disable(void)
{
    if (sd_pin < 0) {
        ESP_LOGE(TAG, "MAX98357 not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    gpio_set_level(sd_pin, 0);
    ESP_LOGI(TAG, "Amplifier disabled");
    return ESP_OK;
}

esp_err_t max98357_write_audio(const void *buf, size_t len, size_t *bytes_written, TickType_t ticks_to_wait)
{
    if (buf == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    return i2s_write(i2s_port, buf, len, bytes_written, ticks_to_wait);
}

esp_err_t max98357_deinit(void)
{
    // Disable amplifier
    if (sd_pin >= 0) {
        gpio_set_level(sd_pin, 0);
    }

    esp_err_t ret = i2s_driver_uninstall(i2s_port);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to uninstall I2S driver: %s", esp_err_to_name(ret));
        return ret;
    }

    sd_pin = -1;
    ESP_LOGI(TAG, "MAX98357 deinitialized");
    return ESP_OK;
}
