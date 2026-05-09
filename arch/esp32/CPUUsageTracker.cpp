#ifdef ESP32
#include "CPUUsageTracker.h"
#include "esp_freertos_hooks.h"

volatile uint32_t CPUUsageTracker::s_idle_ticks = 0;
volatile uint32_t CPUUsageTracker::s_busy_ticks = 0;
TaskHandle_t      CPUUsageTracker::s_idle_handle = nullptr;

void IRAM_ATTR CPUUsageTracker::s_tick_hook() {   // ← was CPUsageTracker (typo)
  if (xTaskGetCurrentTaskHandle() == s_idle_handle) {
    s_idle_ticks++;
  } else {
    s_busy_ticks++;
  }
}

void CPUUsageTracker::s_sample_cb(void* arg) {
  static_cast<CPUUsageTracker*>(arg)->_onSample();  // ← was CpuUsageTracker (old name)
}

void CPUUsageTracker::_onSample() {
  uint32_t busy = s_busy_ticks;
  uint32_t idle = s_idle_ticks;
  uint32_t db = busy - _last_busy;
  uint32_t di = idle - _last_idle;
  _last_busy = busy;
  _last_idle = idle;

  uint32_t total = db + di;
  float sample = (total > 0) ? (float)db / total : 0.0f;

  _avg1  = DECAY1  * _avg1  + (1.0f - DECAY1)  * sample;
  _avg5  = DECAY5  * _avg5  + (1.0f - DECAY5)  * sample;
  _avg15 = DECAY15 * _avg15 + (1.0f - DECAY15) * sample;
}

void CPUUsageTracker::begin() {
  s_idle_handle = xTaskGetIdleTaskHandleForCPU(0);
  esp_register_freertos_tick_hook_for_cpu(s_tick_hook, 0);

  esp_timer_create_args_t args = {
    .callback        = s_sample_cb,
    .arg             = this,
    .dispatch_method = ESP_TIMER_TASK,
    .name            = "cpu_avg"
  };
  esp_timer_create(&args, &_timer);
  esp_timer_start_periodic(_timer, 5000000ULL);  // 5 seconds
}

#endif
