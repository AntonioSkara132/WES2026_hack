#include "udp_client.h"
#include "esp_log.h"


// Initialize and start UDP audio client
void play(void) {
	esp_err_t ret = udp_client_init();
	if (ret != ESP_OK) {
		ESP_LOGE("APP_MAIN", "Failed to initialize UDP client");
		return;
	}

	ret = udp_client_start();
	if (ret != ESP_OK) {
		ESP_LOGE("APP_MAIN", "Failed to start UDP client");
		return;
	}

	ESP_LOGI("APP_MAIN", "UDP audio streaming client started");
}