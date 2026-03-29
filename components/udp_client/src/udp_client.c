#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "audio_utils.h"

// Tag for logging
static const char *TAG = "UDP_AUDIO_CLIENT";

// Wi-Fi configuration
#define WIFI_SSID      "Bird"
#define WIFI_PASS      "BIRD*2025*"
#define WIFI_MAX_RETRY 5

// UDP server settings (the source of the stream)
#define UDP_LISTEN_PORT 8000          // Local port to bind and listen on
#define REMOTE_IP       "172.16.55.104" // Expected sender IP (optional, can be any)
#define REMOTE_PORT     8000           // Expected sender port (optional)

// Buffer size for receiving UDP datagrams (should be large enough for audio chunks)
#define UDP_RX_BUFFER_SIZE 4096

// Event group bits for Wi‑Fi
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static TaskHandle_t udp_receive_task_handle = NULL;
static bool udp_client_running = false;

// Forward declarations
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data);
void wifi_init_sta(void);
void udp_receive_task(void *pvParameters);
esp_err_t udp_client_start(void);

// Wi‑Fi event handler
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "Connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Wi‑Fi initialisation and connection
void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "Wi-Fi init finished.");

    // Wait for connection or failure
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP successfully!");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to AP");
        // You may want to handle this error (e.g., restart)
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

// Static buffer to avoid stack overflow
static uint8_t rx_buffer[UDP_RX_BUFFER_SIZE]; // Receive as bytes

// Task that receives UDP datagrams and plays them as audio
void udp_receive_task(void *pvParameters)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create UDP socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    // Bind to local port
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(UDP_LISTEN_PORT);
    int err = bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Failed to bind UDP socket: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "UDP socket bound to port %d, waiting for audio stream...", UDP_LISTEN_PORT);

    // Sender address info
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    while (udp_client_running) {
        ssize_t received = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0,
                                     (struct sockaddr *)&sender_addr, &sender_len);
        if (received < 0) {
            ESP_LOGE(TAG, "Error receiving UDP packet: errno %d", errno);
            break;
        } else if (received == 0) {
            // UDP doesn't send 0-length packets normally, but just in case
            continue;
        }

        // Convert sender IP to string
        char sender_ip_str[16];
        inet_ntoa_r(sender_addr.sin_addr, sender_ip_str, sizeof(sender_ip_str));
        ESP_LOGI(TAG, "Received %d bytes of audio data from %s:%d",
                 received, sender_ip_str, ntohs(sender_addr.sin_port));

        // Play the received audio data
        // Assume the data is 16-bit PCM samples
        size_t samples_count = received / sizeof(int16_t);
        if (samples_count > 0) {
            // Cast the buffer to int16_t for audio playback
            esp_err_t ret = audio_play_buffer((const int16_t *)rx_buffer, received);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to play audio buffer: %s", esp_err_to_name(ret));
            }
        }
    }

    close(sock);
    ESP_LOGI(TAG, "UDP socket closed, audio receiving task exiting");
    vTaskDelete(NULL);
}

esp_err_t udp_client_init(void)
{
    ESP_LOGI(TAG, "Initializing UDP audio client...");

    // Initialize NVS (required for Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize audio system
    ret = audio_utils_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio system: %s", esp_err_to_name(ret));
        return ret;
    }

    // Optional software gain for weaker input streams (1.0 = no gain)
    esp_err_t gain_ret = audio_utils_set_gain(1.0f); // 4x loudness (increased for better audibility)
    if (gain_ret != ESP_OK) {
        ESP_LOGW(TAG, "Could not set audio gain: %s", esp_err_to_name(gain_ret));
    }

    ESP_LOGI(TAG, "UDP audio client initialized successfully");
    return ESP_OK;
}

esp_err_t udp_client_start_audio_streaming(void)
{
    ESP_LOGI(TAG, "Starting UDP audio streaming...");

    // Initialize the UDP client
    esp_err_t ret = udp_client_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize UDP client: %s", esp_err_to_name(ret));
        return ret;
    }

    // Start the UDP client
    ret = udp_client_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start UDP client: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "UDP audio streaming started successfully");
    return ESP_OK;
}

esp_err_t udp_client_start(void)
{
    if (udp_client_running) {
        ESP_LOGW(TAG, "UDP client is already running");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Starting UDP audio client...");

    // Connect to Wi-Fi
    wifi_init_sta();

    // Set running flag
    udp_client_running = true;

    // Start the UDP receiving task (stack size 4096, priority 5)
    BaseType_t result = xTaskCreate(udp_receive_task, "udp_audio_rx", 4096, NULL, 5, &udp_receive_task_handle);
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UDP receive task");
        udp_client_running = false;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "UDP audio client started successfully");
    return ESP_OK;
}

esp_err_t udp_client_stop(void)
{
    if (!udp_client_running) {
        ESP_LOGW(TAG, "UDP client is not running");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Stopping UDP audio client...");

    // Set running flag to false to stop the task
    udp_client_running = false;

    // Wait for the task to finish (with timeout)
    if (udp_receive_task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(100)); // Give task time to exit gracefully
        // Note: In a real implementation, you might want to use vTaskDelete or proper task synchronization
    }

    // Deinitialize audio system
    esp_err_t ret = audio_utils_deinit();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinitialize audio system: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "UDP audio client stopped");
    return ESP_OK;
}

