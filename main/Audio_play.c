#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "audio_utils.h"

static const char *TAG = "I2S_SPEAKER";

/**
 * @brief Audio playback test task
 * Demonstrates various audio playback scenarios
 */
void audio_test_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting audio playback demonstrations...");
    
    // Example 1: Play a 440 Hz tone (A4 note) for 2 seconds
    ESP_LOGI(TAG, "\n=== Playing 440 Hz sine wave (2 seconds) ===");
    audio_play_sine_wave(880, 2000, 20000);  // 440 Hz, 2 seconds, volume ~60%
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Example 2: Play a sequence of tones (simple melody)
    ESP_LOGI(TAG, "\n=== Playing note sequence ===");
    int notes[] = {880, 988, 1046, 1175}; // A, B, C#, D
    int num_notes = sizeof(notes) / sizeof(notes[0]);
    
    for (int i = 0; i < num_notes; i++) {
        ESP_LOGI(TAG, "Note %d: %d Hz", i+1, notes[i]);
        audio_play_sine_wave(notes[i], 500, 15000);  // 500ms per note
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Example 3: Longer duration test
    ESP_LOGI(TAG, "\n=== Playing 5000 Hz sine wave (1 second) ===");
    audio_play_sine_wave(5000, 2000, 32767);  // 5000 Hz, loud test tone
    
    ESP_LOGI(TAG, "\n=== Audio demonstrations complete ===");
    
    // Delete this task
    vTaskDelete(NULL);
}

/**
 * @brief Application entry point
 */
void audio_play(void)
{
    ESP_LOGI(TAG, "ESP32 MAX98357 I2S Speaker Driver Example");
    ESP_LOGI(TAG, "==========================================");
    
    // Initialize audio subsystem (includes MAX98357 setup)
    esp_err_t ret = audio_utils_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio system");
        return;
    }
    
    ESP_LOGI(TAG, "Audio system initialized successfully");
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Create audio playback task
    xTaskCreatePinnedToCore(
        audio_test_task,
        "audio_test",
        4096,
        NULL,
        5,
        NULL,
        0
    );
}