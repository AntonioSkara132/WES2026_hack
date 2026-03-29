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

static float g_audio_gain = 1.0f;  // Software gain multiplier
static int32_t g_clip_count = 0;   // Counter for clipped samples

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
        .dma_buf_count = 16,       // Increased from 8 for more buffering
        .dma_buf_len = 512         // Increased from 64 to reduce underrun dropouts
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

    if (g_audio_gain != 1.0f) {
        size_t sample_count = buffer_size / sizeof(int16_t);
        int16_t *temp_buf = malloc(buffer_size);
        if (temp_buf == NULL) {
            ESP_LOGW(TAG, "Could not allocate temp buffer for gain; sending raw data");
            goto write_raw;
        }

        int32_t clip_count_this_batch = 0;
        for (size_t i = 0; i < sample_count; ++i) {
            float scaled = buffer[i] * g_audio_gain;
            if (scaled > INT16_MAX) {
                scaled = INT16_MAX;
                clip_count_this_batch++;
            }
            if (scaled < INT16_MIN) {
                scaled = INT16_MIN;
                clip_count_this_batch++;
            }
            temp_buf[i] = (int16_t)scaled;
        }

        if (clip_count_this_batch > 0) {
            g_clip_count += clip_count_this_batch;
            ESP_LOGW(TAG, "Clipping detected: %d samples this batch (total clipped: %ld)", 
                     clip_count_this_batch, g_clip_count);
        }

        size_t bytes_written = 0;
        esp_err_t ret = max98357_write_audio(temp_buf, buffer_size, &bytes_written, portMAX_DELAY);
        free(temp_buf);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write audio buffer: %s", esp_err_to_name(ret));
            return ret;
        }

        ESP_LOGI(TAG, "Wrote %zu bytes to I2S (gain: %.2f)", bytes_written, g_audio_gain);
        return ESP_OK;
    }

write_raw:
    {
        size_t bytes_written = 0;
        esp_err_t ret = max98357_write_audio(buffer, buffer_size, &bytes_written, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write audio buffer: %s", esp_err_to_name(ret));
            return ret;
        }
        ESP_LOGI(TAG, "Wrote %zu bytes to I2S", bytes_written);
        return ESP_OK;
    }
}

esp_err_t audio_utils_set_gain(float gain)
{
    if (gain <= 0.0f) {
        return ESP_ERR_INVALID_ARG;
    }
    g_audio_gain = gain;
    ESP_LOGI(TAG, "Audio gain set to %.2f", g_audio_gain);
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
