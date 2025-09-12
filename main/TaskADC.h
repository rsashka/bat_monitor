#ifndef TASK_ADC_H
#define TASK_ADC_H

#include "TaskConfig.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"

class TaskADC : public RtosTask {
  TaskConfig &m_conf;

  adc_oneshot_unit_handle_t m_adc_handle;
  adc_cali_handle_t m_adc_cali_handle;
  adc_channel_t m_adc_hi;
  adc_channel_t m_adc_lo;
  bool m_do_calibration;
  int adc_raw[2][10];
  int voltage[2][10];

 public:
  TaskADC(TaskConfig &conf);

 protected:
  void setup() override;
  void loop() override;
  void cleanup() override;
};

#endif  // TASK_ADC_H
