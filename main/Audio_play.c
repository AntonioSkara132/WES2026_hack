#include "udp_client.h"
#include "esp_log.h"


// Initialize and start UDP audio client
void play(void) {
	esp_err_t ret = udp_client_start_audio_streaming();
	if (ret != ESP_OK) {
		ESP_LOGE("APP_MAIN", "Failed to start UDP audio streaming");
	}
}