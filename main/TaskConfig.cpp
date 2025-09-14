#include <cstdlib>
#include <regex>

#include "TaskConfig.h"
#include "driver/usb_serial_jtag.h"
#include "esp_system.h"

TaskConfig::TaskConfig()
    : RtosTask("Config", 4048),
      m_prop{
          {STAT,
           {"5", &CheckFloat,
            "Time to update statistics or zero to disable"}},  //
          {HTTP_ALLOW_IP,
           {"", &NotImplemented, "Allows hosts ip to web access"}},           //
          {HTTP_USERNAME, {"", &NotImplemented, "User name to web access"}},  //
          {HTTP_PASSWORD, {"", &NotImplemented, "Password to web access"}},   //

          {HOSTNAME, {"BAT-MONITOR", nullptr, "Hostname"}},                  //
          {WIFI_SSID, {"", nullptr, "WiFi SSID"}},                           //
          {WIFI_MODE, {"WPA2", nullptr, "Valid values are WPA, WPA2"}},      //
          {WIFI_PASS, {"", nullptr, "WiFi password"}},                       //
          {WIFI_RETRY, {"100", &CheckFloat, "Retry to connect to the AP"}},  //
          {WIFI_IP, {"", &NotImplemented, "Empty for use DHCP"}},            //

          {SNTP_SERVER, {"", &NotImplemented, "SNTP server for time sync"}},  //

          {SHUNT_R, {"", &CheckFloat, "Shunt resistance in ohms"}},    //
          {SHUNT_I, {"50", &CheckFloat, "Nominal current for 75mV"}},  //

          {DIV_HI, {"", &CheckFloat, "Upper divisor"}},  //
          {DIV_LO, {"", &CheckFloat, "Lower divisor"}},  //

          {REBOOT, {"", nullptr, "Set reboot to reboot"}},  //

      } {
  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  // Open NVS handle
  ESP_LOGD(m_taskName, "\nOpening Non-Volatile Storage (NVS) handle...");

  err = nvs_open("config", NVS_READWRITE, &m_storage);
  if (err != ESP_OK) {
    ESP_LOGE(m_taskName, "Error (%s) opening NVS handle!",
             esp_err_to_name(err));
  }
};

bool TaskConfig::NotImplemented(const std::string &) {
  ESP_LOGE("", "Not implemented!");
  return false;
}

bool TaskConfig::CheckFloat(const std::string &s) {
  static const std::regex pattern("\\d+(\\.\\d+)?");
  return std::regex_match(s, pattern);
}

const std::string &TaskConfig::get_prop(std::string_view prop) {
  auto found = m_prop.find(prop.begin());
  if (found == m_prop.end()) {
    ESP_LOGE(m_taskName, "Property name '%s' not found!", prop.begin());
    static const std::string nullstr;
    return nullstr;
  }
  return found->second.value;
}

bool TaskConfig::DumpPrint(const std::string &str) {
  esp_log(ESP_LOG_CONFIG_INIT(ESP_LOG_INFO), "", "%s", str.c_str());
  return true;
}

void TaskConfig::Dump(const std::string_view math,
                      PropertyCallbackFunc *callback) {
  std::string hdr;
  hdr = std::format("\nList of keys matching '{}'\n", math.begin());
  (*callback)(hdr);

  if (!callback) {
    ESP_LOGE(m_taskName, "PropertyCallbackFunc not defined!");
    return;
  }

  hdr = "-------------------------------\n";
  (*callback)(hdr);
  for (auto prop : m_prop) {
    if (PropertyParser::matchesPattern(prop.first, math.begin(), false)) {
      (*callback)(std::format("{}='{}' # {}\n", prop.first.c_str(),
                              prop.second.value.c_str(), prop.second.help));
      // esp_log(ESP_LOG_CONFIG_INIT(ESP_LOG_INFO), "", "%s='%s' # %s\n",
      //         prop.first.c_str(), prop.second.value.c_str(),
      //         prop.second.help);
    }
  }
  (*callback)(hdr);
}
bool TaskConfig::PropUpdate(const std::string_view name,
                            const std::string_view new_value) {
  esp_err_t err;
  if (name.compare(SHUNT_I) == 0) {
    // Calculate the resistance of the current shunt 75mV
    m_prop[SHUNT_R].value = std::to_string(0.075f / atof(new_value.begin()));
    ESP_LOGI(m_taskName, "Recalc property '%s' to '%s' Ohm", SHUNT_R,
             m_prop[SHUNT_R].value.c_str());

    err = nvs_set_str(m_storage, SHUNT_R, m_prop[SHUNT_R].value.c_str());
    if (err != ESP_OK) {
      ESP_LOGE(m_taskName, "Failed write '%s' to new value '%s' - %s",
               name.begin(), new_value.begin(), esp_err_to_name(err));
    }

  } else if (name.compare(SHUNT_R) == 0) {
    m_prop[SHUNT_I].value.clear();
    ESP_LOGI(m_taskName, "Reset property '%s'", SHUNT_I);

    err = nvs_set_str(m_storage, SHUNT_I, "");
    if (err != ESP_OK) {
      ESP_LOGE(m_taskName, "Failed reset '%s' - %s", SHUNT_I,
               esp_err_to_name(err));
    }
  } else if (name.compare(STAT) == 0) {
    TaskPool.m_interval = atoi(new_value.begin());
  } else if (name.compare(REBOOT) == 0) {
    if (new_value.compare(REBOOT) == 0) {
      ESP_LOGI(m_taskName, "Start reboot in 1 second!\n\n\n\n");
      sleep(1000);
      esp_restart();
    } else {
      ESP_LOGI(m_taskName, "To reboot, enter 'reboot=reboot'");
    }
    return false;  // Not save property
  }
  return true;
}
void TaskConfig::ParserCallback(void *obj, const PropertyParser &parser) {
  esp_err_t err;
  TaskConfig *config = static_cast<TaskConfig *>(obj);

  assert(config);

  if (parser.isValid()) {
    ESP_LOGD(config->m_taskName, "%s=%s", parser.getPropertyName().c_str(),
             parser.getPropertyValue().c_str());

    auto found = config->m_prop.find(parser.getPropertyName().c_str());
    if (found == config->m_prop.end()) {
      ESP_LOGE(config->m_taskName, "Property name '%s' not found!",
               parser.getPropertyName().c_str());
    } else {
      if (config->m_prop[parser.getPropertyName().c_str()].check == nullptr ||
          (*config->m_prop[parser.getPropertyName().c_str()].check)(
              parser.getPropertyValue())) {
        if (config->PropUpdate(parser.getPropertyName(),
                               parser.getPropertyValue())) {
          err = nvs_set_str(config->m_storage, parser.getPropertyName().c_str(),
                            parser.getPropertyValue().c_str());
          if (err != ESP_OK) {
            ESP_LOGE(config->m_taskName,
                     "Failed write '%s' to new value '%s' - %s",
                     parser.getPropertyName().c_str(),
                     parser.getPropertyValue().c_str(), esp_err_to_name(err));
          }

          config->m_prop[parser.getPropertyName().c_str()].value =
              parser.getPropertyValue().c_str();
          ESP_LOGD(config->m_taskName, "Property name '%s' set vaalue '%s'",
                   parser.getPropertyName().c_str(),
                   parser.getPropertyValue().c_str());

          err = nvs_commit(config->m_storage);
          if (err != ESP_OK) {
            ESP_LOGE(config->m_taskName, "Failed to commit NVS changes!");
          }
        };

      } else {
        ESP_LOGE(config->m_taskName, "Error set value '%s' for property '%s'",
                 parser.getPropertyValue().c_str(),
                 parser.getPropertyName().c_str());
      }
    }
  } else if (!parser.getPropertyMatch().empty()) {
    config->Dump(parser.getPropertyMatch());
  } else {
    ESP_LOGE(config->m_taskName, "Error parse property!");
  }
}

void TaskConfig::setup() {
  esp_err_t err;
  size_t required_size = 0;
  for (auto &prop : m_prop) {
    err = nvs_get_str(m_storage, prop.first.c_str(), NULL, &required_size);
    if (err == ESP_OK) {
      prop.second.value.resize(required_size);

      err = nvs_get_str(m_storage, prop.first.c_str(), prop.second.value.data(),
                        &required_size);
      if (err != ESP_OK) {
        ESP_LOGE(m_taskName, "Fail read %s (%s)", prop.first.c_str(),
                 esp_err_to_name(err));
      } else {
        PropUpdate(prop.first, prop.second.value);
      }
    }
  }

  Dump("*");
}

void TaskConfig::loop() {
  static PropertyParser parser(200, true);
  static char data[200];
  int len = usb_serial_jtag_read_bytes(data, sizeof(data) - 1,
                                       20 / portTICK_PERIOD_MS);
  if (len) {
    parser.feedAndParse(data, len, &TaskConfig::ParserCallback, this);
  }
}

void TaskConfig::cleanup() {
  // Close
  nvs_close(m_storage);
  ESP_LOGD(m_taskName, "NVS handle closed.");
};
