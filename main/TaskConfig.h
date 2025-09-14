#ifndef INCLUDED_TASK_CONFIG_H_
#define INCLUDED_TASK_CONFIG_H_

#include <nvs.h>
#include <nvs_flash.h>

#include <map>
#include <nvs_handle.hpp>
#include <string>

#include "PropertyParser.h"
#include "RtosTask.h"

/*
 */
typedef bool PropertyCallbackFunc(const std::string &);

struct Property {
  std::string value;            // Property value as string
  PropertyCallbackFunc *check;  // Check func before set value
  const char *help;             // Info about property
};
/*

*/
class TaskConfig : public RtosTask {
  std::map<std::string, Property> m_prop;

  nvs_handle_t m_storage;

  static bool CheckFloat(const std::string &);
  static bool NotImplemented(const std::string &);

  static bool DumpPrint(const std::string &);
  void Dump(const std::string_view math,
            PropertyCallbackFunc *callback = &TaskConfig::DumpPrint);

 public:
  static void ParserCallback(void *, const PropertyParser &);
  bool PropUpdate(const std::string_view name, const std::string_view new_value);

 public:
  static constexpr const char *HOSTNAME = "hostname";
  static constexpr const char *WIFI_SSID = "wifi-ssid";
  static constexpr const char *WIFI_MODE = "wifi-mode";
  static constexpr const char *WIFI_PASS = "wifi-pass";
  static constexpr const char *WIFI_RETRY = "wifi-retry";
  static constexpr const char *WIFI_IP = "wifi-ip";

  static constexpr const char *SNTP_SERVER = "sntp-server";

  static constexpr const char *SHUNT_R = "shunt-r";
  static constexpr const char *SHUNT_I = "shunt-i";
  static constexpr const char *DIV_LO = "div-lo";
  static constexpr const char *DIV_HI = "div-hi";

  static constexpr const char *STAT = "stat";
  static constexpr const char *HTTP_ALLOW_IP = "http-allow-ip";
  static constexpr const char *HTTP_USERNAME = "http-username";
  static constexpr const char *HTTP_PASSWORD = "http-password";

  static constexpr const char *REBOOT = "reboot";

  const std::string &get_prop(std::string_view prop);

 public:
  TaskConfig();

 protected:
  void setup() override;
  void loop() override;
  void cleanup() override;
};

#endif  // INCLUDED_TASK_CONFIG_H_