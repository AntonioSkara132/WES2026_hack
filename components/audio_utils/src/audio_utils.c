#include "audio_utils.h"
#include "max98357.h"
#include "esp_log.h"
#include "math.h"
#include <string.h>

static const char *TAG = "AUDIO_UTILS";

#define SAMPLE_RATE 44100
#define BITS_PER_SAMPLE 16
#define NUM_CHANNELS 1
#define BYTES_PER_SAMPLE (BITS_PER_SAMPLE / 8)

esp_err_t audio_utils_init(void)
{
    ESP_LOGI(TAG, "Initializing audio utilities...");
    
    // Configure MAX98357
    max98357_config_t amp_config = {
        .sd_pin = 21,              // GPIO21 for SD_MODE
        .bck_pin = 26,             // I2S_BCLK
        .ws_pin = 25,              // I2S_LRCLK
        .dout_pin = 22,            // I2S TX data
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = BITS_PER_SAMPLE,
        .dma_buf_count = 8,
        .dma_buf_len = 64
    };
    
    esp_err_t ret = max98357_init(&amp_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MAX98357: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Enable amplifier
    ret = max98357_amp_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable amplifier: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Audio utilities initialized");
    return ESP_OK;
}

esp_err_t audio_play_sine_wave(int frequency, int duration_ms, int16_t volume)
{
    if (frequency <= 0 || duration_ms <= 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Playing sine wave: %d Hz for %d ms at volume %d", 
             frequency, duration_ms, volume);
    
    // Calculate number of samples
    int num_samples = (SAMPLE_RATE * duration_ms) / 1000;
    int buffer_size = num_samples * BYTES_PER_SAMPLE;
    
    // Allocate buffer
    int16_t *buffer = (int16_t *)malloc(buffer_size);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer");
        return ESP_ERR_NO_MEM;
    }
    
    // Generate sine wave
    float phase_increment = (2.0f * M_PI * frequency) / SAMPLE_RATE;
    for (int i = 0; i < num_samples; i++) {
        float value = sinf(phase_increment * i) * volume;
        buffer[i] = (int16_t)value;
    }
    
    // Play the buffer
    size_t bytes_written = 0;
    esp_err_t ret = max98357_write_audio(buffer, buffer_size, &bytes_written, portMAX_DELAY);
    
    free(buffer);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write audio data: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Wrote %zu bytes to I2S", bytes_written);
    return ESP_OK;
}

esp_err_t audio_play_buffer(const int16_t *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t bytes_written = 0;
    esp_err_t ret = max98357_write_audio(buffer, buffer_size, &bytes_written, portMAX_DELAY);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write audio buffer: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Wrote %zu bytes to I2S", bytes_written);
    return ESP_OK;
}

esp_err_t audio_utils_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing audio utilities...");
    
    // Disable amplifier and deinit I2S
    esp_err_t ret = max98357_deinit();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinitialize MAX98357: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Audio utilities deinitialized");
    return ESP_OK;
}
