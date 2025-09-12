#ifndef TASK_ADC_H
#define TASK_ADC_H

#include "TaskConfig.h"

class TaskADC : public RtosTask {
  TaskConfig &m_conf;

 public:
  TaskADC(TaskConfig &conf) : RtosTask("ADC", 2048), m_conf(conf) {}

 protected:
  virtual void setup() {}
  virtual void loop() {
    ESP_LOGI("ADC", "test!");
    sleep(1000);
  }
  virtual void cleanup() {}
};

#endif  // TASK_ADC_H
