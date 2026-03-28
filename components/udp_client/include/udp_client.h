#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize and start UDP audio client for streaming
 * This function combines initialization and startup in one call
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t udp_client_start_audio_streaming(void);

/**
 * @brief Initialize UDP client for audio streaming
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t udp_client_init(void);

/**
 * @brief Start UDP audio receiving and playback task
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t udp_client_start(void);

/**
 * @brief Stop UDP audio receiving and playback
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t udp_client_stop(void);

#ifdef __cplusplus
}
#endif

#endif // UDP_CLIENT_H
