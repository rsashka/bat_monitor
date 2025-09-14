#include "TaskServer.h"

/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */
#include <string.h>
#include <sys/unistd.h>

#include "PropertyParser.h"
#include "TaskConfig.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

/* The event group allows multiple bits for each event, but we only care about
 * two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

void TaskServer::event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  TaskServer *task = static_cast<TaskServer *>(arg);

  assert(task);

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
    gpio_set_level(PIN_LED, 1);

  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (task->m_retry_num <
        atoi(task->m_conf.get_prop(TaskConfig::WIFI_RETRY).c_str())) {
      esp_wifi_connect();
      task->m_retry_num++;
      ESP_LOGI(task->m_taskName, "retry to connect to the AP");
    } else {
      xEventGroupSetBits(task->s_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(task->m_taskName, "connect to the AP fail");
    gpio_set_level(PIN_LED, 1);

  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(task->m_taskName, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    task->m_retry_num = 0;
    xEventGroupSetBits(task->s_wifi_event_group, WIFI_CONNECTED_BIT);

    gpio_set_level(PIN_LED, 0);
  }
}

void TaskServer::wifi_init_sta(void) {
  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());

  esp_netif_t *esp_netif = esp_netif_create_default_wifi_sta();
  ESP_ERROR_CHECK(esp_netif_set_hostname(
      esp_netif, m_conf.get_prop(TaskConfig::HOSTNAME).c_str()));

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, this, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, this, &instance_got_ip));

  wifi_config_t wifi_config = {};

  if (m_conf.get_prop(TaskConfig::WIFI_MODE).empty() ||
      m_conf.get_prop(TaskConfig::WIFI_MODE).compare("OPEN") == 0) {
    // None
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
  } else if (m_conf.get_prop(TaskConfig::WIFI_MODE).compare("WEP") == 0) {
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WEP;
  } else if (m_conf.get_prop(TaskConfig::WIFI_MODE).compare("WPA2") == 0) {
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  } else {
    ESP_LOGE(m_taskName,
             "Unknown or not implemented Wi-Fi authentication method (%s)",
             m_conf.get_prop(TaskConfig::WIFI_MODE).c_str());
  }

  std::copy(m_conf.get_prop(TaskConfig::WIFI_SSID).begin(),
            m_conf.get_prop(TaskConfig::WIFI_SSID).end(), wifi_config.sta.ssid);
  std::copy(m_conf.get_prop(TaskConfig::WIFI_PASS).begin(),
            m_conf.get_prop(TaskConfig::WIFI_PASS).end(),
            wifi_config.sta.password);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGD(m_taskName, "wifi_init_sta finished.");

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or
   * connection failed for the maximum number of re-tries (WIFI_FAIL_BIT). The
   * bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we
   * can test which event actually happened. */
  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(m_taskName, "connected to ap SSID:%s",
             m_conf.get_prop(TaskConfig::WIFI_SSID).c_str());

  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGI(m_taskName, "Failed to connect to SSID:%s, password:%s",
             m_conf.get_prop(TaskConfig::WIFI_SSID).c_str(),
             m_conf.get_prop(TaskConfig::WIFI_PASS).c_str());
  } else {
    ESP_LOGE(m_taskName, "UNEXPECTED EVENT");
  }
}

// HTTP request handler for root
// Dynamic HTML content generator
esp_err_t TaskServer::dynamic_handler(httpd_req_t *req) {
  // Get the TaskServer instance from user_ctx
  TaskServer *task = static_cast<TaskServer *>(req->user_ctx);
  assert(task);

  // Set response type
  httpd_resp_set_type(req, "text/html");

  // HTML content template (static string to minimize memory usage)
  const char *html_first =
      "<!DOCTYPE html>"
      "<html lang=\"ru\">"
      "<head>"
      ""
      "<meta charset=utf-8>"
      "<title>";

  httpd_resp_sendstr_chunk(req, html_first);
  httpd_resp_sendstr_chunk(req,
                           task->m_conf.get_prop(TaskConfig::HOSTNAME).c_str());

  const char *html_second =
      "</title>"
      "<style>"
      "body { font-family: Arial, sans-serif; margin: 20px; }"
      "</style>\n"
      "</head>\n"
      ""
      "<body>"
      "<h1>";

  httpd_resp_sendstr_chunk(req, html_second);
  httpd_resp_sendstr_chunk(req,
                           task->m_conf.get_prop(TaskConfig::HOSTNAME).c_str());

  const char *html_body =
      "</h1>"
      "<div id=\"info-container\">"
      "<p id=\"jsDisabled\">JavaScript disabled.</p>"
      "</div>"
      "\n"
      "<label for='name'>Property: </label>"
      "<input type='text' id='input-data' name='input-data' size='40' />"
      "<div><textarea id='query-data' readonly></textarea></div>"
      "\n"
      ""
      "<script>\n"
      ""
      "const infoContainer = document.getElementById('info-container');"
      "const queryData = document.getElementById('query-data');"
      "\n"
      ""
      ""
      "function startInfoTimer() {"
      "  var request = new XMLHttpRequest();"
      "  request.open('GET', 'http://' + window.location.host + '/info');"
      "  request.responseType = 'text';"
      "  request.onload = function () {"
      "    infoContainer.innerHTML = request.response;"
      "  };"
      "  request.send();\n"
      ""
      "  setTimeout(startInfoTimer,1000);"
      "}\n"
      ""
      "document.addEventListener('DOMContentLoaded', function() {"
      " var query = document.getElementById('input-data');"
      " query.addEventListener('keypress', function(e) {"
      "   if (e.keyCode == 13) {"
      "     var request = new XMLHttpRequest();"
      "     request.open('GET', 'http://' + window.location.host + "
      "       '/query?'+document.getElementById('input-data').value);"
      "     request.responseType = 'text';"
      "     request.onload = function () {"
      "       queryData.value = request.response;"
      "     };"
      "     request.send();\n"
      "   }"
      " });"
      "});"
      "\n"
      "document.getElementById('jsDisabled').style.visibility='hidden';\n"
      "startInfoTimer();\n"
      ""
      "</script>\n"
      "</body>"
      "</html>";

  // Send complete HTML content
  httpd_resp_sendstr_chunk(req, html_body);
  httpd_resp_sendstr_chunk(req, nullptr);
  return ESP_OK;
}

/*
 */
esp_err_t TaskServer::info_handler(httpd_req_t *req) {
  // Get the TaskServer instance from user_ctx
  TaskServer *task = static_cast<TaskServer *>(req->user_ctx);

  // Set response type
  httpd_resp_set_type(req, "text/html");

  httpd_resp_sendstr_chunk(req, "<ul>");

  httpd_resp_sendstr_chunk(req, "<li>IDF version: <b>");
  httpd_resp_sendstr_chunk(req, esp_get_idf_version());
  httpd_resp_sendstr_chunk(req, "</b></li><br>");

  httpd_resp_sendstr_chunk(req, "<li>Locat time: <b>");
  time_t t = std::time(nullptr);
  httpd_resp_sendstr_chunk(req, ctime(&t));
  httpd_resp_sendstr_chunk(req, "</b></li>");

  httpd_resp_sendstr_chunk(req, "<li>Uptime: <b>");
  int32_t uptime = esp_timer_get_time() / 1000000;
  int32_t days = uptime / 86400;
  httpd_resp_sendstr_chunk(
      req, std::format("{}d {}h {}m {}s", days, (uptime - days) / 3600,
                       (uptime / 60 % 60), uptime % 60)
               .c_str());
  httpd_resp_sendstr_chunk(req, "</b></li><br>");

  httpd_resp_sendstr_chunk(req, "<li>High voltage: <b>");
  httpd_resp_sendstr_chunk(
      req, std::format("{:.3f}", task->m_adc->GetHiVoltage()).c_str());
  httpd_resp_sendstr_chunk(req, "</b></li>");
  httpd_resp_sendstr_chunk(req, "<li>Low voltage: <b>");
  httpd_resp_sendstr_chunk(
      req, std::format("{:.3f}", task->m_adc->GetLoVoltage()).c_str());
  httpd_resp_sendstr_chunk(req, "</b></li>");
  httpd_resp_sendstr_chunk(req, "<li>Current: <b>");
  httpd_resp_sendstr_chunk(
      req, std::format("{:.2f}", task->m_adc->GetСurrent()).c_str());
  httpd_resp_sendstr_chunk(req, "</b></li>");

  httpd_resp_sendstr_chunk(req, "</ul>");
  httpd_resp_sendstr_chunk(req, nullptr);

  return ESP_OK;
}

std::string server_log;
extern "C" int server_logger(const char *fmt, va_list args) {
  char msg_to_send[300];
  const size_t str_len = vsnprintf(msg_to_send, 299, fmt, args);
  server_log.append(msg_to_send, str_len);
  return str_len;
}

esp_err_t TaskServer::query_handler(httpd_req_t *req) {
  // Get the TaskServer instance from user_ctx
  TaskServer *task = static_cast<TaskServer *>(req->user_ctx);

  // Set response type
  httpd_resp_set_type(req, "text/html");

  static PropertyParser parser(200, true);
  static char param[150];

  esp_err_t err = httpd_req_get_url_query_str(req, param, sizeof(param) - 2);
  if (err == ESP_OK) {
    // httpd_resp_sendstr_chunk(req, "<textarea>");

    server_log.clear();
    vprintf_like_t save = esp_log_set_vprintf(server_logger);
    // ESP_LOGI(task->m_taskName, "Query: %s", param);

    size_t len = strlen(param);
    param[len] = '\n';
    param[len + 1] = '\0';
    parser.feedAndParse(param, strlen(param), &TaskConfig::ParserCallback,
                        &task->m_conf);

    esp_log_set_vprintf(save);

    httpd_resp_sendstr_chunk(req, server_log.c_str());

    // httpd_resp_sendstr_chunk(req, "</textarea>");
    httpd_resp_sendstr_chunk(req, nullptr);

  } else {
    httpd_resp_send(req, nullptr, 0);
  }

  return ESP_OK;
}

httpd_handle_t TaskServer::setup_server(void) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 8;
  config.lru_purge_enable = true;
  httpd_handle_t server = NULL;

  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_register_uri_handler(server, &m_root_uri);
    httpd_register_uri_handler(server, &m_info_uri);
    httpd_register_uri_handler(server, &m_query_uri);
    ESP_LOGI(m_taskName, "HTTP server started");
  }

  return server;
}

TaskServer::TaskServer(TaskConfig &conf, TaskADC *adc)
    : RtosTask("SRV", 8000), m_conf(conf), m_adc(adc) {
  m_retry_num = 0;
  m_server = nullptr;

  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if ((err == ESP_ERR_NVS_NO_FREE_PAGES) ||
      (err == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    //        ESP_LOGW("NVS", "Erasing NVS partition...");
    ESP_LOGI(m_taskName, "Erasing NVS partition...\n");
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  // Register URI handlers
  m_root_uri = {.uri = "/",
                .method = HTTP_GET,
                .handler = dynamic_handler,
                .user_ctx = this};

  m_info_uri = {.uri = "/info",
                .method = HTTP_GET,
                .handler = info_handler,
                .user_ctx = this};

  m_query_uri = {.uri = "/query",
                 .method = HTTP_GET,
                 .handler = query_handler,
                 .user_ctx = this};

  gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
  gpio_set_level(PIN_LED, 1);
}

void TaskServer::setup() {
  wifi_init_sta();
  setup_server();
}

void TaskServer::loop() { sleep(0); }
void TaskServer::cleanup() {
  // Ensure handle is non NULL
  if (m_server != NULL) {
    // Stop the httpd server
    httpd_stop(m_server);
  }
}

/*

https://www.10-strike.ru/network-monitor/ibp-ups.shtml
https://www.ixbt.com/live/supply/rabota-s-ibp-po-snmp-na-primere-oborudovaniya-powercom.html

В параметрах проверки SNMP укажите пароль для доступа к ИБП (community, если
SNMP v1,2c или логин/пароль, если SNMP v3).

При помощи встроенного MIB-браузера выберите нужный OID, чтобы мониторить
определенный параметр устройства. Например:

    1.3.6.1.2.1.33.1.2.2.0 — Время работы ИБП от батарей, в секундах (ноль
означает, что ИБП питается от сети) 1.3.6.1.2.1.33.1.2.3.0 — Время до истощения
заряда батарей при отключенном электропитании, в минутах 1.3.6.1.2.1.33.1.2.4.0
— Оставшийся заряд батарей, в процентах 1.3.6.1.2.1.33.1.2.5.0 — Текущее
напряжение батареи, в десятых долях Вольт 1.3.6.1.2.1.33.1.2.6.0 — Текущий ток
батареи, в десятых долях Ампер 1.3.6.1.2.1.33.1.2.7.0 — Температура ИБП, в
Цельсиях
*/

// void send_custom_trap_example(void);
//
// static void snmp_agent_task(void *pvParameters) {
//     ESP_LOGI(m_taskName, "Starting SNMP agent task");
//     // Initialize SNMP agent
//     snmp_init(); // This initializes MIB2 stuff as well if enabled
//
//     // Set community strings (these might also be set via menuconfig)
//     // If set via menuconfig (CONFIG_LWIP_SNMP_COMMUNITY,
//     CONFIG_LWIP_SNMP_COMMUNITY_WRITE),
//     // they are typically compiled in. Calling these functions overrides
//     them. snmp_set_community((const char*) CONFIG_LWIP_SNMP_COMMUNITY);
//     snmp_set_community_write((const char*) CONFIG_LWIP_SNMP_COMMUNITY_WRITE);
//
//     // Enable authentication for SET requests using the write community
//
////    snmp_set_write_access(SNMP_VERSION_2c); // Or SNMP_VERSION_1, ensure
/// this matches your manager !!!!!!!!!!!!!!!!!!
//
//    // For SNMPv2c, the set_community_write is used.
//    // For SNMPv1, it's also set_community_write.
//    // It implies that if a packet comes with the write community, it's
//    allowed.
//
//    // Initialize our custom MIB
//    custom_mib_init();
//
//    ESP_LOGI(m_taskName, "SNMP Agent initialized. Ready to receive
//    requests."); ESP_LOGI(m_taskName, "Read Community: %s",
//    CONFIG_LWIP_SNMP_COMMUNITY); ESP_LOGI(m_taskName, "Write Community: %s",
//    CONFIG_LWIP_SNMP_COMMUNITY_WRITE);
//
//
//    // Periodically send a trap as an example
//    uint32_t trap_counter = 0;
//    while (1) {
//        vTaskDelay(pdMS_TO_TICKS(30000)); // Every 30 seconds
//        ESP_LOGI(m_taskName, "Sending example trap (%lu)...", trap_counter++);
//        send_custom_trap_example();
//    }
//    vTaskDelete(NULL);
//}

// void send_custom_trap_example(void) {
//     // Define a specific trap OID under our enterprise:
//     .1.3.6.1.4.1.99999.2.1
//     // (.1.3.6.1.4.1.yourEnterpriseID.yourTrapsGroup.yourSpecificTrapID)
//     // For traps, the last sub-identifier is often 0.
//     // The second to last is the specific trap number.
//     // The enterprise OID itself is usually passed as part of the trap PDU.
//
//     // Generic trap type for enterprise specific traps
//     #define SNMP_TRAP_ENTERPRISE_SPECIFIC 6
//
//     // Our specific trap ID (e.g., 1 for "device restart", 2 for "sensor
//     alert") u32_t specific_trap_id = 1; // Example: Sensor Alert Trap
//
//     // The OID that defines this trap. This is usually your enterprise OID +
//     a traps sub-identifier + specific trap id.
//     // For example: .1.3.6.1.4.1.99999 (enterprise) .2 (traps group) .1
//     (specific trap)
//     // This full OID is sent as the "snmpTrapOID.0" varbind in an SNMPv2c
//     trap. oid_t trap_oid_val[] = {CUSTOM_ENTERPRISE_OID_BASE, 2,
//     specific_trap_id}; // .1.3.6.1.4.1.99999.2.1 struct snmp_obj_id trap_oid;
//     SNMP_OBJ_ID_SET(&trap_oid, trap_oid_val, LWIP_ARRAYSIZE(trap_oid_val));
//
//
//     // Optional: Add variable bindings (varbinds) to the trap PDU to provide
//     more context struct snmp_varbind *vb = snmp_varbind_alloc(&trap_oid,
//     SNMP_ASN1_TYPE_OCTET_STRING, strlen("Sensor threshold exceeded")); if (vb
//     == NULL) {
//         ESP_LOGE(m_taskName, "Failed to allocate varbind for trap");
//         return;
//     }
//     memcpy(vb->value, "Sensor threshold exceeded", strlen("Sensor threshold
//     exceeded"));
//
//     // Add another varbind, e.g., current temperature that caused the alert
//     // For this, we'd need the OID of our temperature sensor
//     oid_t temp_oid_val[] = {CUSTOM_ENTERPRISE_OID_BASE, PRODUCT_GROUP_OID,
//     MODULE_GROUP_OID, DEVICE_STATUS_OID_LEAF, 0}; // Using device status OID
//     for example struct snmp_obj_id temp_obj_oid;
//     SNMP_OBJ_ID_SET(&temp_obj_oid, temp_oid_val,
//     LWIP_ARRAYSIZE(temp_oid_val));
//
//     s32_t current_temp_val = 35; // Example value
//     struct snmp_varbind *vb_temp = snmp_varbind_alloc(&temp_obj_oid,
//     SNMP_ASN1_TYPE_INTEGER, sizeof(s32_t)); if(vb_temp == NULL) {
//         ESP_LOGE(m_taskName, "Failed to allocate temp varbind for trap");
//         snmp_varbind_free(vb); // Free previously allocated varbind
//         return;
//     }
//     *(s32_t*)vb_temp->value = current_temp_val;
//     vb->next = vb_temp; // Link varbinds
//
//     ESP_LOGI(m_taskName, "Sending SNMP Trap for specific ID %lu",
//     specific_trap_id);
//     // For SNMPv2c/v3 traps, snmpTrapOID.0 is the first varbind.
//     // The uptime is also automatically included as sysUpTimeInstance.0.
//     // The enterprise OID used in snmp_send_trap_specific is often the
//     agent's enterprise root. oid_t enterprise_oid_val[] =
//     {CUSTOM_ENTERPRISE_OID_BASE}; struct snmp_obj_id enterprise_obj_oid;
//     SNMP_OBJ_ID_SET(&enterprise_obj_oid, enterprise_oid_val,
//     LWIP_ARRAYSIZE(enterprise_oid_val));
//
//     // For SNMPv2c, the trap OID itself is sent as the first varbind.
//     // The `snmp_send_trap_specific` is more for SNMPv1 style traps if not
//     careful.
//     // For v2c, we should use `snmp_send_trap_ext` or build PDU manually.
//     // lwIP's snmp_trap() or snmp_send_trap_specific() might be v1 oriented.
//     // Let's try a more direct approach for SNMPv2 TRAP if available or a
//     standard v1 trap.
//     // The lwIP API `snmp_send_trap_specific` is for SNMPv1.
//     // To send SNMPv2c trap, you usually set
//     `snmp_trap_varbind_enterprise_oid = 0`
//     // and provide the trap OID as the first varbind.
//
//     // For simplicity, we'll use the generic `snmp_trap` which should adapt
//     based on context or default to v1/v2c basics.
//     // The modern way is `snmp_send_trap_ext(SNMP_TRAP_ENTERPRISE_SPECIFIC,
//     specific_trap_id, &enterprise_obj_oid, vb);`
//     // However, snmp_send_trap_ext might not be available or easy to use in
//     all lwIP versions/configs directly.
//     // The `snmp_send_trap_specific` is more for SNMPv1.
//     // The `snmp_send_inform` is for INFORMs.
//     // `snmp_v2_trap_ex` seems to be the internal function.
//     //
//     // The easiest way to send an SNMPv2c TRAP with specific OID and varbinds
//     with lwIP's default agent:
//     // 1. The first varbind MUST be sysUpTime.0 (SNMP_MSG_OID_SYSUPTIME, 0) -
//     usually added automatically by snmp_send_trap()
//     // 2. The second varbind MUST be snmpTrapOID.0 (SNMP_MSG_OID_SNMPTRAPOID,
//     0) with the value of your trap OID.
//     // 3. Subsequent varbinds are your custom data.
//
//     // Let's construct the varbind list for an SNMPv2c TRAP.
//     // snmp_varbind_list_free(&vb_list_head); // Clear any previous
//     // struct snmp_varbind *vb_list_head = NULL;
//     // struct snmp_varbind *vb_list_tail = NULL;
//
//     // // Varbind 1: snmpTrapOID.0 (Value is our trap OID:
//     .1.3.6.1.4.1.99999.2.1)
//     // oid_t snmptrapoid_oid_val[] = {SNMP_MSG_OID_SNMPTRAPOID, 0};
//     // struct snmp_obj_id snmptrapoid_objid;
//     // SNMP_OBJ_ID_SET(&snmptrapoid_objid, snmptrapoid_oid_val,
//     LWIP_ARRAYSIZE(snmptrapoid_oid_val));
//     // struct snmp_varbind *vbs_trapoid =
//     snmp_varbind_alloc(&snmptrapoid_objid, SNMP_ASN1_TYPE_OBJECT_ID,
//     trap_oid.len * sizeof(oid_t));
//     // if(!vbs_trapoid) { ESP_LOGE(m_taskName, "Failed to alloc snmpTrapOID.0
//     varbind"); return; }
//     // SNMP_OBJ_ID_COPY((struct snmp_obj_id *)vbs_trapoid->value, &trap_oid);
//     // Value is the trap OID itself
//     // snmp_varbind_tail_add(&vb_list_head, &vb_list_tail, vbs_trapoid);
//
//     // // Varbind 2: Custom varbind (e.g., alert message)
//     // // OID: .1.3.6.1.4.1.99999.1.1.0 (esp32DeviceStatus, using it as an
//     example payload OID)
//     // oid_t alert_msg_oid_val[] = {CUSTOM_ENTERPRISE_OID_BASE,
//     PRODUCT_GROUP_OID, MODULE_GROUP_OID, DEVICE_STATUS_OID_LEAF, 0};
//     // struct snmp_obj_id alert_msg_objid;
//     // SNMP_OBJ_ID_SET(&alert_msg_objid, alert_msg_oid_val,
//     LWIP_ARRAYSIZE(alert_msg_oid_val));
//     // const char *alert_str = "High Temperature Alert!";
//     // struct snmp_varbind *vbs_alert_msg =
//     snmp_varbind_alloc(&alert_msg_objid, SNMP_ASN1_TYPE_OCTET_STRING,
//     strlen(alert_str));
//     // if(!vbs_alert_msg) { ESP_LOGE(m_taskName, "Failed to alloc alert_msg
//     varbind"); snmp_varbind_list_free(&vb_list_head); return; }
//     // MEMCPY(vbs_alert_msg->value, alert_str, strlen(alert_str));
//     // snmp_varbind_tail_add(&vb_list_head, &vb_list_tail, vbs_alert_msg);
//
//     // ESP_LOGI(m_taskName, "Sending SNMPv2c Trap with specific OID and
//     varbinds");
//     // snmp_send_trap(SNMP_GENTRAP_ENTERPRISE_SPECIFIC, &enterprise_obj_oid,
//     specific_trap_id, vb_list_head);
//     // snmp_varbind_list_free(&vb_list_head);
//
//     // The above is complex. For a simpler trap using SNMPv1 style (often
//     what basic lwIP `snmp_send_trap_specific` does): ESP_LOGI(m_taskName,
//     "Sending SNMPv1-style Trap (specific %lu)", specific_trap_id);
//     snmp_send_trap_specific(
//         SNMP_TRAP_ENTERPRISE_SPECIFIC, // generic trap type = 6 for
//         enterpriseSpecific specific_trap_id,              // specific trap
//         code &enterprise_obj_oid,           // enterprise OID
//         (.1.3.6.1.4.1.99999) vb                             // list of
//         varbinds (can be NULL)
//     );
//
//     // Free the varbind list if it was allocated (vb points to the head)
//     if (vb != NULL) {
//         snmp_varbind_list_free(&vb);
//     }
// }

// Add new client socket
// void TaskServer::set_socket(int sock) {
//   if (sock == -1) {
//     ESP_LOGE(m_taskName, "Fail new socket connection");
//     return;
//   }

//   struct sockaddr_in addr_in;
//   socklen_t addrlen = sizeof(addr_in);

//   if (lwip_getpeername(sock, (sockaddr *)&addr_in, &addrlen) != -1) {
//     ESP_LOGI(m_taskName, "Client connected from %s",
//              inet_ntoa(addr_in.sin_addr));
//     if (m_socket != -1) {
//       ESP_LOGI(m_taskName, "Old socket %d will be closed!", m_socket);
//     }
//     m_socket = sock;
//   } else {
//     ESP_LOGE(m_taskName, "Fail remote IP is %s",
//     inet_ntoa(addr_in.sin_addr));
//   }
// }
