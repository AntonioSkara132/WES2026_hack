// tcp_helper.h (optional header)
#ifndef TCP_HELPER_H
#define TCP_HELPER_H

#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

// Wi‑Fi configuration
#define WIFI_SSID      "Bird"
#define WIFI_PASS      "BIRD*2025*"
#define WIFI_MAX_RETRY 5

// TCP server settings
#define TCP_SERVER_IP   "172.16.55.106"
#define TCP_SERVER_PORT 8000

// Buffer sizes
#define TCP_RX_BUFFER_SIZE 10000
#define MAX_SEND_MSG_LEN   256

// Return codes
typedef enum {
    TCP_HELPER_OK = 0,
    TCP_HELPER_ERR_WIFI_FAIL,
    TCP_HELPER_ERR_SOCKET_CREATE,
    TCP_HELPER_ERR_CONNECT,
    TCP_HELPER_ERR_SEND,
    TCP_HELPER_ERR_RECV,
    TCP_HELPER_ERR_QUEUE
} tcp_helper_err_t;

// Global queue for outgoing messages (extern so main can access)
extern QueueHandle_t *xSendQueue;
extern QueueHandle_t *xRecvQueue;

// Initialise Wi‑Fi (blocks until connected or max retries)
tcp_helper_err_t wifi_helper_init(void);

// Create a TCP socket and connect to server
// Returns socket fd on success, negative on error
int tcp_client_connect(void);

// Receive data from TCP socket into buffer (non‑blocking with timeout)
// Returns number of bytes received, or -1 on error, 0 if connection closed
ssize_t tcp_client_receive(int sock, uint8_t *buffer, size_t buffer_len, int timeout_ms);

// Send data over TCP socket
// Returns number of bytes sent, or -1 on error
ssize_t tcp_client_send(int sock, const uint8_t *data, size_t len);

// Example task that handles both receiving and sending from queue
void tcp_helper_task(void *pvParameters);

// Optional: user input task that reads from stdin and queues messages
void user_input_task(void *pvParameters);

void init_tcp_task(QueueHandle_t *sendQueue, QueueHandle_t *recvQueue);

#ifdef __cplusplus
}
#endif

#endif // TCP_HELPER_H