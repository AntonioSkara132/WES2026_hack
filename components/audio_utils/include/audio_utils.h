#ifndef AUDIO_UTILS_H
#define AUDIO_UTILS_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize audio playback system
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t audio_utils_init(void);

/**
 * @brief Generate and play a sine wave test tone
 * @param frequency Frequency in Hz (e.g., 440 for A4)
 * @param duration_ms Duration in milliseconds
 * @param volume Volume level (0-32767, where 32767 is maximum)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t audio_play_sine_wave(int frequency, int duration_ms, int16_t volume);

/**
 * @brief Play audio from a PCM buffer
 * @param buffer Audio buffer containing PCM samples
 * @param buffer_size Size of buffer in bytes
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t audio_play_buffer(const int16_t *buffer, size_t buffer_size);

/**
 * @brief Set software gain multiplier (1.0 = unity, >1 = louder)
 * @param gain Floating-point gain factor
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t audio_utils_set_gain(float gain);

/**
 * @brief Deinitialize audio playback system
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t audio_utils_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_UTILS_H
