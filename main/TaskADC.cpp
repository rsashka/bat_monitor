
#include "TaskADC.h"

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "soc/soc_caps.h"
// #include "esp_log.h"

// #include "driver/usb_serial_jtag.h"
// #include "sdkconfig.h"

// #include <string>
// #include <vector>
// #include <format>

// #include "config.h"
// #include "server.h"

// const static char *TAG = "ADC";

// /*---------------------------------------------------------------
//         ADC General Macros
// ---------------------------------------------------------------*/
// // ADC1 Channels
// #define EXAMPLE_ADC1_CHAN0 ADC_CHANNEL_0
// #define EXAMPLE_ADC1_CHAN1 ADC_CHANNEL_1

// #define EXAMPLE_ADC_ATTEN ADC_ATTEN_DB_2_5
// // ADC_ATTEN_DB_12

// static int adc_raw[2][10];
// static int voltage[2][10];
// static bool example_adc_calibration_init(adc_unit_t unit, adc_atten_t atten,
// adc_cali_handle_t *out_handle); static void
// example_adc_calibration_deinit(adc_cali_handle_t handle);

// /*
//  * На чипах ESP со встроенным в кристалл USB Serial/JTAG Controller можно
//  использовать часть этого контроллера,
//  * которая реализует последовательный порт (CDC), для работы последовательной
//  консоли вместо того,
//  * чтобы использовать для этой цели UART вместе с внешней микросхемой моста
//  USB-UART (наподобие CH340x, FT2232RL и т. п.).
//  *
//  * ESP32-C3 содержит на кристалле такой контроллер USB, предоставляющий
//  следующие функции:
//  * • Двунаправленная последовательная консоль, которую можно использовать как
//  IDF Monitor или другой serial-монитор для отладочных сообщений.
//  * • Прошивка FLASH с помощью утилиты esptool.py и команды idf.py flash [2].
//  * • Отладка JTAG с использованием например OpenOCD, одновременно с работой
//  serial-консоли.
//  *
//  * Обратите внимание, что в отличие от некоторых чипов Espressif с
//  периферийным устройством USB OTG,
//  * контроллер USB Serial/JTAG это устройство с фиксированной функцией,
//  реализованной полностью аппаратно.
//  * Это значит, что USB Serial/JTAG Controller нельзя переконфигурировать на
//  использование других функций,
//  * кроме как каналов обмена последовательными сообщениями (USB CDC) и отладки
//  JTAG.
//  */

// #define BUF_SIZE (1024)
// #define ECHO_TASK_STACK_SIZE (4096)

// void send_to_all_clients(const char *data, size_t len = -1);

// void OUT(const std::string &str)
// {
//     usb_serial_jtag_write_bytes(str.data(), str.length(), 20 /
//     portTICK_PERIOD_MS);
//     // send_to_all_clients(msg_to_send, str_len);
// }

// int custom_logger(const char *fmt, va_list args)
// {
//     char msg_to_send[300];
//     const size_t str_len = vsnprintf(msg_to_send, 299, fmt, args);
//     send_to_all_clients(msg_to_send, str_len);
//     // usb_serial_jtag_write_bytes(msg_to_send, str_len);
//     vprintf(fmt, args);
//     return str_len;
// }

// extern "C" void app_main(void)
// {

//     // esp_log_set_vprintf(custom_logger);

//     //--------- Configure USB SERIAL JTAG
//     usb_serial_jtag_driver_config_t usb_serial_jtag_config = {
//         .tx_buffer_size = BUF_SIZE,
//         .rx_buffer_size = BUF_SIZE,
//     };

//     ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_serial_jtag_config));
//     ESP_LOGI("usb_serial_jtag echo", "USB_SERIAL_JTAG init done");

//     // Configure a temporary buffer for the incoming data
//     uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
//     if (data == NULL)
//     {
//         ESP_LOGE("usb_serial_jtag echo", "no memory for data");
//         return;
//     }

//     //-------------ADC1 Init---------------//
//     adc_oneshot_unit_handle_t adc1_handle;

// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
//     adc_oneshot_unit_init_cfg_t init_config1 = {
//         unit_id : ADC_UNIT_1,
//     };
//     ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

//     //-------------ADC1 Config---------------//
//     adc_oneshot_chan_cfg_t config = {
//         atten : EXAMPLE_ADC_ATTEN,
//         bitwidth : ADC_BITWIDTH_DEFAULT,
//     };
// #pragma GCC diagnostic pop

//     ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle,
//     EXAMPLE_ADC1_CHAN0, &config));
//     ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle,
//     EXAMPLE_ADC1_CHAN1, &config));

//     //-------------ADC1 Calibration Init---------------//
//     adc_cali_handle_t adc1_cali_handle = NULL;
//     bool do_calibration1 = example_adc_calibration_init(ADC_UNIT_1,
//     EXAMPLE_ADC_ATTEN, &adc1_cali_handle); int ratio = 26; // (300+12)/12

//     std::string out_str;
//     Config conf;

//     ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
//     wifi_init_sta();

//     // GPIO initialization ESP32-C3
//     //    gpio_pad_select_gpio(LED_PIN);
//     gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
//     gpio_set_direction(PIN_ANSWER, GPIO_MODE_OUTPUT);

//     led_state = 0;
//     ESP_LOGI(TAG, "LED Control Web Server is running ... ...\n");
//     setup_server();

//     while (1)
//     {

//         out_str.clear();
//         int len = usb_serial_jtag_read_bytes(data, (BUF_SIZE - 1), 20 /
//         portTICK_PERIOD_MS); if (conf.Process(data, len, out_str))
//         {
//             usb_serial_jtag_write_bytes(out_str.data(), out_str.length(), 20
//             / portTICK_PERIOD_MS); gpio_set_level(PIN_ANSWER, 1);
//             gpio_set_level(PIN_ANSWER, 0);
//         }

//         //        // Write data back to the USB SERIAL JTAG
//         //        if (len) {
//         //            usb_serial_jtag_write_bytes((const char *) data, len,
//         20 / portTICK_PERIOD_MS);
//         //            data[len] = '\0';
//         //            ESP_LOG_BUFFER_HEXDUMP("Recv str: ", data, len,
//         ESP_LOG_INFO);
//         //        }

//         ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN0,
//         &adc_raw[0][0])); ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d",
//         ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN0, adc_raw[0][0]);
//         OUT(std::format("ADC{} Channel[{}] Raw Data: {}\n", ADC_UNIT_1 + 1,
//         (int)EXAMPLE_ADC1_CHAN0, adc_raw[0][0])); if (do_calibration1)
//         {
//             ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle,
//             adc_raw[0][0], &voltage[0][0])); ESP_LOGI(TAG, "ADC%d Channel[%d]
//             Cali Voltage: %d mV", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN0, ratio *
//             voltage[0][0]); OUT(std::format("ADC{} Channel[{}] Cali Voltage:
//             {} mV\n", ADC_UNIT_1 + 1, (int)EXAMPLE_ADC1_CHAN0, ratio *
//             voltage[0][0]));
//         }
//         vTaskDelay(pdMS_TO_TICKS(1000));

//         ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN1,
//         &adc_raw[0][1])); ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d",
//         ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN1, adc_raw[0][1]);
//         OUT(std::format("ADC{} Channel[{}] Raw Data: {}\n", ADC_UNIT_1 + 1,
//         (int)EXAMPLE_ADC1_CHAN1, adc_raw[0][1])); if (do_calibration1)
//         {
//             ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle,
//             adc_raw[0][1], &voltage[0][1])); ESP_LOGI(TAG, "ADC%d Channel[%d]
//             Cali Voltage: %d mV", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN1, ratio *
//             voltage[0][1]); OUT(std::format("ADC{} Channel[{}] Cali Voltage:
//             {} mV\n", ADC_UNIT_1 + 1, (int)EXAMPLE_ADC1_CHAN1, ratio *
//             voltage[0][1]));
//         }
//         vTaskDelay(pdMS_TO_TICKS(1000));

//         ESP_LOGI(TAG, "ADC%d diff Raw Data: %d", ADC_UNIT_1 + 1,
//         adc_raw[0][1] - adc_raw[0][0]); OUT(std::format("ADC{} diff Raw Data:
//         {}\n", ADC_UNIT_1 + 1, adc_raw[0][1] - adc_raw[0][0])); if
//         (do_calibration1)
//         {
//             ESP_LOGI(TAG, "ADC%d diff Voltage: %d mV", ADC_UNIT_1 + 1, ratio
//             * (voltage[0][1] - voltage[0][0])); OUT(std::format("ADC{} diff
//             Voltage: {} mV\n", ADC_UNIT_1 + 1, ratio * (voltage[0][1] -
//             voltage[0][0])));
//         }
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }

//     // Tear Down
//     ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
//     if (do_calibration1)
//     {
//         example_adc_calibration_deinit(adc1_cali_handle);
//     }
// }

/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_atten_t atten,
                                         adc_cali_handle_t *out_handle) {
  adc_cali_handle_t handle = NULL;
  esp_err_t ret = ESP_FAIL;
  bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
  if (!calibrated) {
    // ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
  if (!calibrated) {
    ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

  *out_handle = handle;
  if (ret == ESP_OK) {
    // ESP_LOGI(TAG, "Calibration Success");
  } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
    // ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
  } else {
    // ESP_LOGE(TAG, "Invalid arg or no memory");
  }

  return calibrated;
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
//   ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
  ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
  ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
  ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}

TaskADC::TaskADC(TaskConfig &conf) : RtosTask("ADC", 2048), m_conf(conf) {
  adc_oneshot_unit_init_cfg_t init_config = {};
  init_config.unit_id = ADC_UNIT_1;
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &m_adc_handle));

  m_adc_hi = ADC_CHANNEL_0;
  m_adc_lo = ADC_CHANNEL_1;
}

void TaskADC::setup() {
  //-------------ADC1 Config---------------//
  adc_oneshot_chan_cfg_t config = {
    atten : ADC_ATTEN_DB_2_5,
    bitwidth : ADC_BITWIDTH_DEFAULT,
  };

  ESP_ERROR_CHECK(adc_oneshot_config_channel(m_adc_handle, m_adc_hi, &config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(m_adc_handle, m_adc_lo, &config));

  //-------------ADC1 Calibration Init---------------//
  m_adc_cali_handle = nullptr;
  m_do_calibration = example_adc_calibration_init(ADC_UNIT_1, ADC_ATTEN_DB_2_5,
                                                  &m_adc_cali_handle);
}

void TaskADC::loop() {
  int ratio = 26;  // (300+12)/12

  ESP_ERROR_CHECK(adc_oneshot_read(m_adc_handle, m_adc_hi, &adc_raw[0][0]));
  ESP_LOGI(m_taskName, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1,
           m_adc_hi, adc_raw[0][0]);

  if (m_do_calibration) {
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(m_adc_cali_handle, adc_raw[0][0],
                                            &voltage[0][0]));
    ESP_LOGI(m_taskName, "ADC%d Channel[%d] Cali Voltage : %d mV ",
             ADC_UNIT_1 + 1, m_adc_hi, ratio * voltage[0][0]);
  }
  sleep(1000);

  ESP_ERROR_CHECK(adc_oneshot_read(m_adc_handle, m_adc_lo, &adc_raw[0][1]));
  ESP_LOGI(m_taskName, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1,
           m_adc_lo, adc_raw[0][1]);

  if (m_do_calibration) {
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(m_adc_cali_handle, adc_raw[0][1],
                                            &voltage[0][1]));
    ESP_LOGI(m_taskName, "ADC%d Channel[%d] Cali Voltage : %d mV ",
             ADC_UNIT_1 + 1, m_adc_lo, ratio * voltage[0][1]);
  }
  sleep(1000);

  ESP_LOGI(m_taskName, "ADC%d diff Raw Data: %d", ADC_UNIT_1 + 1,
           adc_raw[0][1] - adc_raw[0][0]);
  if (m_do_calibration) {
    ESP_LOGI(m_taskName, "ADC%d diff Voltage: %d mV", ADC_UNIT_1 + 1,
             ratio * (voltage[0][1] - voltage[0][0]));
  }

  sleep(1000);
}

void TaskADC::cleanup() {
  // Tear Down
  ESP_ERROR_CHECK(adc_oneshot_del_unit(m_adc_handle));
  if (m_do_calibration) {
    example_adc_calibration_deinit(m_adc_cali_handle);
  }
}
