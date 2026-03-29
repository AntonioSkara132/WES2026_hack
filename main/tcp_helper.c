// tcp_helper.c
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "tcp_helper.h"
#include "buzzer.h"

static const char *TAG = "TCP_HELPER";

// Event group bits for Wi‑Fi
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

// Queue for outgoing messages (public)
QueueHandle_t *xSendQueue = NULL;
QueueHandle_t *xRecvQueue = NULL;

// -------------------------------------------------------------------
// Wi‑Fi event handler (internal)
// -------------------------------------------------------------------
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

// -------------------------------------------------------------------
// Public Wi‑Fi initialisation
// -------------------------------------------------------------------
tcp_helper_err_t wifi_helper_init(void)
{
    // Initialise NVS if not already done
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

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
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi init finished.");

    // Wait for connection
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP successfully!");
        return TCP_HELPER_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to AP");
        return TCP_HELPER_ERR_WIFI_FAIL;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        return TCP_HELPER_ERR_WIFI_FAIL;
    }
}

// -------------------------------------------------------------------
// TCP client connect
// -------------------------------------------------------------------
int tcp_client_connect(void)
{
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create TCP socket: errno %d", errno);
        return -1;
    }

    // Set up server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TCP_SERVER_PORT);
    if (inet_pton(AF_INET, TCP_SERVER_IP, &server_addr.sin_addr) != 1) {
        ESP_LOGE(TAG, "Invalid server IP address: %s", TCP_SERVER_IP);
        close(sock);
        return -1;
    }

    ESP_LOGI(TAG, "Connecting to TCP server %s:%d", TCP_SERVER_IP, TCP_SERVER_PORT);
    int err = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Failed to connect to TCP server: errno %d", errno);
        close(sock);
        return -1;
    }
    ESP_LOGI(TAG, "Connected to TCP server");

    return sock;
}

// -------------------------------------------------------------------
// TCP receive with timeout (non‑blocking, uses SO_RCVTIMEO)
// -------------------------------------------------------------------
ssize_t tcp_client_receive(int sock, uint8_t *buffer, size_t buffer_len, int timeout_ms)
{
    // Set receive timeout if requested
    if (timeout_ms >= 0) {
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    ssize_t received = recv(sock, buffer, buffer_len, 0);
    if (received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Timeout, no data
            return 0;
        } else {
            ESP_LOGE(TAG, "recv error: errno %d", errno);
            return -1;
        }
    } else if (received == 0) {
        // Connection closed
        return 0;
    }

    return received;
}

// -------------------------------------------------------------------
// TCP send
// -------------------------------------------------------------------
ssize_t tcp_client_send(int sock, const uint8_t *data, size_t len)
{
    ssize_t sent = send(sock, data, len, 0);
    if (sent < 0) {
        ESP_LOGE(TAG, "send error: errno %d", errno);
    }
    return sent;
}

// -------------------------------------------------------------------
// Hex dump helper (optional, for printing received data)
// -------------------------------------------------------------------
static void print_hex_dump(const char *prefix, const uint8_t *data, size_t len)
{
    char line_buf[80];
    size_t offset = 0;
    while (offset < len) {
        size_t bytes_this_line = len - offset;
        if (bytes_this_line > 16) bytes_this_line = 16;

        char *ptr = line_buf;
        ptr += sprintf(ptr, "%s %04zx: ", prefix, offset);
        for (size_t i = 0; i < bytes_this_line; i++) {
            ptr += sprintf(ptr, "%02x ", data[offset + i]);
        }
        for (size_t i = bytes_this_line; i < 16; i++) {
            ptr += sprintf(ptr, "   ");
        }
        ESP_LOGI(TAG, "%s", line_buf);
        offset += bytes_this_line;
    }
}

// -------------------------------------------------------------------
// Combined TCP task (receives and sends queued messages)
// -------------------------------------------------------------------
void tcp_helper_task(void *pvParameters)
{
    static uint8_t rx_buffer[TCP_RX_BUFFER_SIZE];
    int sock;

    while (1) {
        // Attempt to connect
        sock = tcp_client_connect();
        if (sock < 0) {
            ESP_LOGE(TAG, "Connection failed, retry in 5s");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // Main loop: receive with timeout (100 ms) and send queued messages
        while (1) {
            // Try to receive data (timeout 100 ms)
            ssize_t received = tcp_client_receive(sock, rx_buffer, sizeof(rx_buffer), 100);

	    
            if (received > 0) {	      	     
                ESP_LOGI(TAG, "Received %d bytes", received);
                print_hex_dump("TCP", rx_buffer, received);
                buzzer_beep(100); // Beep on receive
                buzzer_beep(100); // Beep on receive
                

		if (*xRecvQueue != NULL) {
		  if (xQueueSend(*xRecvQueue, rx_buffer, 0) != pdTRUE) {
		    ESP_LOGW(TAG, "Recv queue full, dropping data");
		  }
		}
		
            } else if (received < 0) {
                ESP_LOGE(TAG, "Receive error, reconnecting...");
                break;
            } // else 0 means timeout or closed, we handle send below

            // Send any pending messages from queue
            char send_buf[64];
            while (xQueueReceive(*xSendQueue, send_buf, 0) == pdTRUE) {
                //size_t len = strlen(send_buf);
                ssize_t sent = tcp_client_send(sock, (uint8_t *)send_buf, 64);
                if (sent != 64) {
                    ESP_LOGE(TAG, "Send error, reconnecting...");
                    break;
                } else {
                    ESP_LOGI(TAG, "Sent %d bytes", sent);

                }
            }

            vTaskDelay(pdMS_TO_TICKS(10));
        }

        close(sock);
        ESP_LOGI(TAG, "Connection lost, reconnecting in 5s...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// -------------------------------------------------------------------
// User input task (reads from serial monitor)
// -------------------------------------------------------------------
void user_input_task(void *pvParameters)
{
    char line[MAX_SEND_MSG_LEN];

    while (1) {
        if (fgets(line, sizeof(line), stdin) != NULL) {

            // Remove newline if present
            line[strcspn(line, "\n")] = 0;

            // Create fixed 64-byte buffer
            char tx_buf[64] = {0};

            size_t len = strlen(line);
            if (len > 64) len = 64;

            memcpy(tx_buf, line, len);

            // Pad with spaces
            for (int i = len; i < 64; i++) {
                tx_buf[i] = ' ';
            }

            // Send EXACTLY 64 bytes into queue
            if (xQueueSend(*xSendQueue, tx_buf, pdMS_TO_TICKS(1000)) != pdTRUE) {
                ESP_LOGW(TAG, "Send queue full, dropping message");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(NULL);
}


void init_tcp_task(QueueHandle_t *sendQueue, QueueHandle_t *recvQueue)
{
    xSendQueue = sendQueue;
    xRecvQueue = recvQueue;
    buzzer_init(); // Ensure buzzer is ready for use in TCP task
    //wifi_helper_init(); 

    xTaskCreate(tcp_helper_task, "tcp_task", 4096, NULL, 4, NULL);
    xTaskCreate(user_input_task, "user_input_task", 4096, NULL, 5, NULL);
}
