#ifndef INCLUDED_SERVER_H_
#define INCLUDED_SERVER_H_

#include <esp_http_server.h>
#include <lwip/api.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "RtosTask.h"
#include "driver/gpio.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "lwip/err.h"
#include "lwip/sockets.h"  // For getting IP address
#include "lwip/sys.h"

//// SNMP includes
// #include "lwip/apps/snmp_opts.h" // For LWIP_SNMP
//
// #include "lwip/apps/snmp.h"
// #include "lwip/apps/snmp_core.h"
// #include "lwip/apps/snmp_mib2.h"
////#include "lwip/apps/snmp_agent.h" // For snmp_set_community_write etc.
//
// #include "lwip/apps/snmp_opts.h"
// #include "lwip/apps/snmp_scalar.h"
// #include "lwip/apps/snmp_snmpv2_framework.h"
// #include "lwip/apps/snmp_snmpv2_usm.h"
// #include "lwip/apps/snmp_table.h"
// #include "lwip/apps/snmp_threadsync.h"
//
//
//// Define our private enterprise OID base
////
///.iso(1).org(3).dod(6).internet(1).private(4).enterprise(1).yourEnterpriseID(99999)
// #define CUSTOM_ENTERPRISE_OID_BASE {1, 3, 6, 1, 4, 1, 99999}
// #define CUSTOM_ENTERPRISE_OID_LEN 7 // Length of the base OID
//
// #define CONFIG_LWIP_SNMP_COMMUNITY "SNMP_COMMUNITY"
// #define CONFIG_LWIP_SNMP_COMMUNITY_WRITE "SNMP_COMMUNITY_WRITE"
//
//// Our custom MIB objects under this base
//// .1.3.6.1.4.1.99999.1.1.0 : esp32DeviceStatus (OCTET_STRING, RO)
//// .1.3.6.1.4.1.99999.1.2.0 : esp32Uptime (TIMETICKS, RO)
//// .1.3.6.1.4.1.99999.1.3.0 : esp32LedState (INTEGER, RW)
//
//// These are leaf identifiers relative to
///CUSTOM_ENTERPRISE_OID_BASE.productGroup.moduleGroup / For simplicity, let's
///assume productGroup = 1, moduleGroup = 1 / So, esp32DeviceStatus will be
///CUSTOM_ENTERPRISE_OID_BASE + .1.1.0 / esp32Uptime will be
///CUSTOM_ENTERPRISE_OID_BASE + .1.2.0 / esp32LedState will be
///CUSTOM_ENTERPRISE_OID_BASE + .1.3.0
//
//
//// Structure to hold the state for our custom MIB objects (e.g., LED state)
// typedef struct {
//     int32_t led_state; // 0 for OFF, 1 for ON
//     // Add other states here if needed
// } custom_mib_states_t;
//
// extern custom_mib_states_t g_custom_mib_states;
//
// void custom_mib_init(void);
//
//// Optional: function to simulate an event and send a trap
// void send_custom_trap_example(void);

#include "TaskADC.h"
#include "TaskConfig.h"

#define PIN_LED (gpio_num_t)8
#define PIN_ANSWER (gpio_num_t)6
#define PIN_INPUT (gpio_num_t)7
#define PIN_OUT (gpio_num_t)10
#define PIN_DS (gpio_num_t)2
#define PIN_KEY (gpio_num_t)9

#define PIN_ADCO 0
#define PIN_ADC1 1

extern int led_state;
void wifi_init_sta(void);
httpd_handle_t setup_server(void);


extern "C" int server_logger(const char *fmt, va_list args);


class TaskServer : public RtosTask {
 private:
  /* FreeRTOS event group to signal when we are connected*/
  EventGroupHandle_t s_wifi_event_group;
  int m_retry_num;

  httpd_handle_t m_server = NULL;

 protected:
  // int m_socket;
  // httpd_ws_frame_t m_send_frame;

  // Register URI handlers
  httpd_uri_t m_root_uri;
  httpd_uri_t m_info_uri;
  httpd_uri_t m_query_uri;

  TaskConfig & m_conf;
  TaskADC *m_adc;

 public:
  TaskServer(TaskConfig &conf, TaskADC *adc);

  TaskServer(const TaskServer &) = default;
  TaskServer(TaskServer &&) = default;
  TaskServer &operator=(const TaskServer &) = delete;
  TaskServer &operator=(TaskServer &&) = delete;

 protected:
  void setup() override;
  void loop() override;
  void cleanup() override;

  static void event_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data);
  void wifi_init_sta();
  // void set_socket(int sock);

  // HTTP request handler for root
  // Dynamic HTML content generator
  static esp_err_t dynamic_handler(httpd_req_t *req);

  static esp_err_t info_handler(httpd_req_t *req);

  // Submit handler
  static esp_err_t query_handler(httpd_req_t *req);

  httpd_handle_t setup_server();
};

#endif  // INCLUDED_SERVER_H_
