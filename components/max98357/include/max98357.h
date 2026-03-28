#ifndef MAX98357_H
#define MAX98357_H

#include "driver/i2s.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MAX98357 amplifier configuration structure
 */
typedef struct {
    int sd_pin;           /*!< GPIO pin for SD_MODE (speaker enable/disable) */
    int bck_pin;          /*!< I2S_BCLK pin */
    int ws_pin;           /*!< I2S_LRCLK (word select) pin */
    int dout_pin;         /*!< I2S TX data pin */
    int sample_rate;      /*!< Sample rate (e.g., 44100, 48000) */
    int bits_per_sample;  /*!< Bits per sample (8, 16, 24, 32) */
    int dma_buf_count;    /*!< Number of DMA buffers */
    int dma_buf_len;      /*!< DMA buffer length */
} max98357_config_t;

/**
 * @brief Initialize MAX98357 amplifier and I2S peripheral
 * @param config Pointer to configuration structure
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t max98357_init(const max98357_config_t *config);

/**
 * @brief Enable the MAX98357 amplifier (pull SD_MODE high)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t max98357_amp_enable(void);

/**
 * @brief Disable the MAX98357 amplifier (pull SD_MODE low)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t max98357_amp_disable(void);

/**
 * @brief Write audio data to I2S for playback
 * @param buf Pointer to audio buffer (PCM data)
 * @param len Number of bytes to write
 * @param bytes_written Pointer to store number of bytes actually written
 * @param ticks_to_wait Ticks to wait if buffer is full (use portMAX_DELAY for blocking)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t max98357_write_audio(const void *buf, size_t len, size_t *bytes_written, TickType_t ticks_to_wait);

/**
 * @brief Deinitialize MAX98357 and I2S peripheral
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t max98357_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // MAX98357_H
