#include <stdio.h>
#include <stdarg.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"

#define TAG "MAIN"
#define UART_PORT UART_NUM_0
#define UART_BAUD_RATE 115200
#define UART_RX_PIN GPIO_NUM_3
#define UART_TX_PIN GPIO_NUM_1
#define UART_BUF_SIZE 1024

// UART initialization
void init_uart()
{
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_LOGI(TAG, "UART initialized");
}

// WebSocket client management
#define MAX_CLIENTS 4
static httpd_handle_t server = NULL;
static int client_sockets[MAX_CLIENTS] = {-1, -1, -1, -1};

// Add client to list
static void add_client(int sock)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (client_sockets[i] == -1)
        {
            client_sockets[i] = sock;
            ESP_LOGI(TAG, "Client %d connected", sock);
            return;
        }
    }
    ESP_LOGW(TAG, "No free slot for new client");
}

// Remove client from list
static void remove_client(int sock)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (client_sockets[i] == sock)
        {
            client_sockets[i] = -1;
            ESP_LOGI(TAG, "Client %d disconnected", sock);
            return;
        }
    }
}

// Send data to all connected clients
static void send_to_all_clients(const char *data, size_t len)
{
    httpd_ws_frame_t ws_pkt = {
        .payload = (uint8_t *)data,
        .len = len,
        .type = HTTPD_WS_TYPE_TEXT};

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (client_sockets[i] != -1)
        {
            if (httpd_ws_send_frame_async(server, client_sockets[i], &ws_pkt) != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to send to client %d", client_sockets[i]);
                remove_client(client_sockets[i]);
            }
        }
    }
}

// UART read task
static void uart_read_task(void *pvParameters)
{
    uint8_t data[UART_BUF_SIZE];
    while (1)
    {
        int len = uart_read_bytes(UART_PORT, data, UART_BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            // Send received data to all WebSocket clients
            char *data_str = malloc(len + 1);
            if (data_str)
            {
                memcpy(data_str, data, len);
                data_str[len] = '\0';
                send_to_all_clients(data_str, len);
                free(data_str);
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// HTTP request handler for root
// Dynamic HTML content generator
static esp_err_t dynamic_handler(httpd_req_t *req)
{
    // Set response type
    httpd_resp_set_type(req, "text/html");

    // HTML content template (static string to minimize memory usage)
    const char *html_template =
        "<!DOCTYPE html>"
        "<html lang=\"ru\">"
        "<head>"
        "<meta charset=utf-8>"
        "<title>ESP32 UART Monitor</title>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 20px; }"
        "#log-container { "
        "height: 300px; "
        "overflow-y: auto; "
        "border: 1px solid #ccc; "
        "padding: 10px; "
        "margin-bottom: 20px;"
        "background-color: #f8f8f8;"
        "}"
        ".log-entry { margin-bottom: 5px; }"
        "form { margin-bottom: 20px; }"
        "textarea { width: 100%; height: 100px; padding: 10px; font-family: inherit; font-size: 1em; }"
        "button { padding: 10px 20px; font-size: 1em; }"
        "</style>"
        "</head>"
        "<body>"
        "<h1>ESP32 UART Monitor</h1>"
        "<div id=\"info-container\"></div>"
        "<div id=\"log-container\"></div>"
        "<form id=\"data-form\" action=\"/submit\" method=\"POST\">"
        "<label for=\"multi-line-input\">Введите данные:</label>"
        "<textarea id=\"multi-line-input\" name=\"data\" placeholder=\"Введите текст...\" required></textarea>"
        "<button type=\"submit\">Отправить</button>"
        "</form>"
        "<button id=\"clear-btn\">Clear Logs</button>"
        "<button id=\"test-btn\">TEST</button>"
        "<script>"
        "const infoContainer = document.getElementById('info-container');"
        "const logContainer = document.getElementById('log-container');"
        "const clearBtn = document.getElementById('clear-btn');"
        "let ws;"
        ""
        "function connectWebSocket() {"
        "ws = new WebSocket('ws://' + window.location.host + '/ws');"
        ""
        "ws.onopen = () => {"
        "addLogEntry('WebSocket connected');"
        "};"
        ""
        "ws.onmessage = (event) => {"
        "addLogEntry(event.data);"
        "};"
        ""
        "ws.onclose = () => {"
        "addLogEntry('WebSocket disconnected. Reconnecting...');"
        "setTimeout(connectWebSocket, 2000);"
        "};"
        ""
        "ws.onerror = (error) => {"
        "console.error('WebSocket error:', error);"
        "};"
        "}"
        ""
        "function addLogEntry(message) {"
        "const entry = document.createElement('div');"
        "entry.className = 'log-entry';"
        "entry.textContent = message;"
        "logContainer.appendChild(entry);"
        "logContainer.scrollTop = logContainer.scrollHeight;"
        "}"
        ""
        "clearBtn.addEventListener('click', () => {"
        "logContainer.innerHTML = '';"
        "});"
        "const testBtn = document.getElementById('test-btn');"
        "if (testBtn) {"
        "testBtn.addEventListener('click', function() {"
        "addLogEntry('Привет мир от ESP32');"
        "});"
        "}"
        ""
        "function startInfoTimer() {"
        "var request = new XMLHttpRequest();"
        "request.open('GET', 'http://' + window.location.host + '/info');"
        "request.responseType = 'text';"
        "request.onload = function () {"
        "infoContainer.innerHTML = request.response;"
        "};"
        "request.send();"
        "setTimeout(startInfoTimer,1000);"
        "}"
        ""
        "connectWebSocket();"
        "startInfoTimer();"
        ""
        "</script>"
        "</body>"
        "</html>";

    // Send complete HTML content
    httpd_resp_send(req, html_template, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t info_handler(httpd_req_t *req)
{
    // Set response type
    httpd_resp_set_type(req, "text/html");

    // HTML content template (static string to minimize memory usage)
    const char *info_body =
        "<ul>"
        "<li>JavaScript</li>"
        "<li>Python</li>"
        "<li>Swift</li>"
        "<li>C#</li>"
        "<li>Go</li>"
        "</ul>"
        "";

    // Send complete HTML content
    httpd_resp_send(req, info_body, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// WebSocket handler
static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
    {
        add_client(httpd_req_to_sockfd(req));
        return ESP_OK;
    }

    int sock = httpd_req_to_sockfd(req);
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

    // Receive WebSocket frame
    if (httpd_ws_recv_frame(req, &ws_pkt, 0) != ESP_OK)
    {
        ESP_LOGE(TAG, "WebSocket receive failed");
        remove_client(sock);
        return ESP_FAIL;
    }

    if (ws_pkt.len)
    {
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL)
        {
            ESP_LOGE(TAG, "Failed to allocate memory");
            remove_client(sock);
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        if (httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len) != ESP_OK)
        {
            ESP_LOGE(TAG, "WebSocket receive failed");
            free(buf);
            remove_client(sock);
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "Received WS message: %s", ws_pkt.payload);
        free(buf);
    }

    // Handle client disconnect
    if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE)
    {
        remove_client(sock);
    }

    return ESP_OK;
}

// Submit handler
static esp_err_t form_handler(httpd_req_t *req)
{
    if (req->method == HTTP_POST)
    {
        // Read form data
        char *buf = malloc(req->content_len);
        if (!buf)
        {
            ESP_LOGE(TAG, "Failed to allocate memory for form data");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        int received = httpd_req_recv(req, buf, req->content_len);
        if (received <= 0)
        {
            ESP_LOGE(TAG, "Failed to receive form data");
            free(buf);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        // Parse form data to extract the 'data' field
        // We'll search for "data=" in the form data
        char *data_start = strstr(buf, "data=");
        if (data_start)
        {
            // Skip past "data="
            data_start += 5;

            // URL decode the data (basic handling for + to space)
            char *data_ptr = data_start;
            char *out_ptr = data_start;
            while (*data_ptr)
            {
                if (*data_ptr == '+')
                {
                    *out_ptr = ' ';
                }
                else
                {
                    *out_ptr = *data_ptr;
                }
                data_ptr++;
                out_ptr++;
            }
            *out_ptr = '\0';

            // Log received data using ESP32 logging function
            ESP_LOGI(TAG, "Received form data: %s", data_start);

            // Send success response
            httpd_resp_send(req, "Data received and logged", HTTPD_RESP_USE_STRLEN);
            free(buf);
            return ESP_OK;
        }
        else
        {
            ESP_LOGW(TAG, "Form data does not contain 'data' field");
            httpd_resp_send(req, "Invalid form data", HTTPD_RESP_USE_STRLEN);
            free(buf);
            return ESP_FAIL;
        }
    }

    // Handle GET request (show form again)
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// Register URI handlers
static httpd_uri_t root_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = dynamic_handler,
    .user_ctx = NULL};

static httpd_uri_t info_uri = {
    .uri = "/info",
    .method = HTTP_GET,
    .handler = info_handler,
    .user_ctx = NULL};

static httpd_uri_t ws_uri = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = ws_handler,
    .user_ctx = NULL,
    .is_websocket = true};

static httpd_uri_t form_uri = {
    .uri = "/submit",
    .method = HTTP_POST,
    .handler = form_handler,
    .user_ctx = NULL};

// Start HTTP server
void start_webserver()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    config.lru_purge_enable = true;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &ws_uri);
        httpd_register_uri_handler(server, &form_uri);
        httpd_register_uri_handler(server, &info_uri);
        ESP_LOGI(TAG, "HTTP server started");
    }
}

void wifi_init_softap()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "ESP32-UART-Server",
            .password = "",
            .ssid_len = strlen("ESP32-UART-Server"),
            .channel = 6,
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN},
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WiFi AP started. SSID: %s", "ESP32-UART-Server");
}

int custom_logger(const char *fmt, va_list args)
{
    char msg_to_send[300];
    const size_t str_len = vsnprintf(msg_to_send, 299, fmt, args);
    send_to_all_clients(msg_to_send, str_len);
    vprintf(fmt, args);
    return str_len;
}

void app_main()
{

    esp_log_set_vprintf(custom_logger);

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize WiFi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_softap();

    // Start HTTP server
    start_webserver();

    // Initialize UART
    init_uart();

    // Start UART read task
    xTaskCreate(uart_read_task, "uart_read_task", 4096, NULL, 10, NULL);
}