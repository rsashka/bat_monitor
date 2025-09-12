#ifndef RTOS_TASK_H
#define RTOS_TASK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#include "esp_log.h"
#include "esp_log_level.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "unittest.h"

class RtosTaskPool;

class RtosTask {
  friend class RtosTaskPool;

  SCOPE(protected)

  RtosTaskPool *m_pool;
  const char *const m_taskName;
  const uint16_t m_stackDepth;
  UBaseType_t m_taskPriority;

  bool m_stopTask = false;
  TaskHandle_t m_handleTask = 0;

 public:
  RtosTask(const char *const taskName, const uint16_t stackDepth,
           UBaseType_t taskPriority = 0)
      : m_taskName(taskName),
        m_stackDepth(stackDepth),
        m_taskPriority(taskPriority) {}

  virtual ~RtosTask() {}

  inline void stop() { this->m_stopTask = true; }

  SCOPE(protected)

  inline void sleep(int time_ms) { vTaskDelay(time_ms / portTICK_PERIOD_MS); }

  SCOPE(private)

  virtual void setup() {}
  virtual void loop() { sleep(0); }
  virtual void cleanup() {}

  static void bootstrap(void *args) {
    RtosTask *taskObject = reinterpret_cast<RtosTask *>(args);
    taskObject->setup();
    while (!taskObject->m_stopTask) {
      taskObject->loop();
    }
    taskObject->cleanup();
    vTaskDelete(taskObject->m_handleTask);
  }
};

class RtosTaskPool {
  std::vector<RtosTask *>
      m_taskPool;  // todo: replace to std::inplace_vector in C++26
  static constexpr const char *m_logTag = "Stat";

 public:
  size_t m_interval;

  RtosTaskPool(const size_t poolSize) : m_taskPool(0), m_interval(5) {
    assert(poolSize);
    // Reserve all memory for the thread pool once when the object is created
    m_taskPool.reserve(poolSize);
  }
  virtual ~RtosTaskPool() {
    // for (auto &&task : m_taskPool) {
    //     delete task;
    //     task = nullptr;
    // }
  }

  void Create(RtosTask *task) {
    // Do not reallocate thread pool memory
    assert(m_taskPool.size() < m_taskPool.capacity());
    m_taskPool.push_back(task);
  }

  inline char _task_state_to_char(eTaskState state) {
    switch (state) {
      case eRunning:
        return '+';
      case eReady:
        return 'R';
      case eBlocked:
        return 'B';
      case eSuspended:
        return 'S';
      case eDeleted:
        return 'D';
      default:
        return '0' + state;
    }
  }

  void Run() {
    // There is at least one thread
    assert(m_taskPool.size());

    for (auto &&task : m_taskPool) {
      assert(task != nullptr);
      task->m_pool = this;
      task->m_stopTask = false;
      xTaskCreate(RtosTask::bootstrap, task->m_taskName, task->m_stackDepth,
                  task, task->m_taskPriority, &task->m_handleTask);
    }

    std::vector<TaskStatus_t>
        task_status_array;  // todo: replace to std::inplace_vector in C++26

    while (true) {
      if (m_interval && esp_log_level_get(m_logTag) >= ESP_LOG_DEBUG) {
        task_status_array.resize(uxTaskGetNumberOfTasks());

        ESP_LOGD(m_logTag, "================= Task Stats =================");

        uint32_t total_run_time = 0;
        const int task_count =
            uxTaskGetSystemState(task_status_array.data(),
                                 task_status_array.size(), &total_run_time);

        assert(task_count <= task_status_array.size());

        // Calculate uptime in milliseconds
        const uint32_t uptime_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

        ESP_LOGD(m_logTag, "Name & st,\tPRI,\tstack,\tused %% (counter)");
        for (int i = 0; i < task_status_array.size(); i++) {
          const uint32_t runtime_counter =
              task_status_array[i].ulRunTimeCounter;

          // Calculate percentage and fractional part
          const uint32_t percentage =
              (total_run_time > 0) ? (runtime_counter * 100 / total_run_time)
                                   : 0;
          const uint32_t fractional =
              (total_run_time > 0) ? ((runtime_counter * 100 % total_run_time) *
                                      100 / total_run_time)
                                   : 0;

          ESP_LOGD(m_logTag, "%10s: %c, \t%3d, \t%5ld, \t%2ld.%02ld%% (%ld)",
                   task_status_array[i].pcTaskName,
                   _task_state_to_char(task_status_array[i].eCurrentState),
                   task_status_array[i].uxCurrentPriority,
                   task_status_array[i].usStackHighWaterMark, percentage,
                   fractional, runtime_counter);
        }

        ESP_LOGD(m_logTag, "----------------------------------------------");
        ESP_LOGD(m_logTag, "Total:  %ld ms", total_run_time);
        ESP_LOGD(m_logTag, "Uptime: %ld ms", uptime_ms);
        ESP_LOGD(m_logTag, "HeapCurr: %d", xPortGetFreeHeapSize());
        ESP_LOGD(m_logTag, "HeapMin:  %d", xPortGetMinimumEverFreeHeapSize());
        ESP_LOGD(m_logTag, "==============================================");
      }
      vTaskDelay(m_interval ? m_interval * 1000 : 1000 / portTICK_PERIOD_MS);
    }
  }
};

extern RtosTaskPool TaskPool;

#endif  // RTOS_TASK_H
