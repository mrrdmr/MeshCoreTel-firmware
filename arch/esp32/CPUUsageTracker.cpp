#ifdef ESP32
#include "CPUUsageTracker.h"
#include "esp_freertos_hooks.h"

volatile uint32_t CPUUsageTracker::s_idle_ticks = 0;
volatile uint32_t CPUUsageTracker::s_busy_ticks = 0;
TaskHandle_t      CPUUsageTracker::s_idle_handle = nullptr;

void IRAM_ATTR CPUUsageTracker::s_tick_hook() {
  if (xTaskGetCurrentTaskHandle() == s_idle_handle) {
    s_idle_ticks++;
  } else {
    s_busy_ticks++;
  }
}

void CPUUsageTracker::s_sample_cb(void* arg) {
  static_cast<CPUUsageTracker*>(arg)->_onSample();
}

void CPUUsageTracker::_onSample() {
  const uint32_t busy = s_busy_ticks;
  const uint32_t idle = s_idle_ticks;
  const uint32_t db = busy - _last_busy;
  const uint32_t di = idle - _last_idle;
  _last_busy = busy;
  _last_idle = idle;

  const uint32_t total = db + di;
  const float sample = (total > 0) ? (float)db / (float)total : 0.0f;

  const uint8_t s8 = (uint8_t)(sample * 255.0f + 0.5f);
  const uint16_t last = _sma_buf[_sma_idx];
  _sma_buf[_sma_idx] = s8;
  _sma_sum = (_sma_sum - last) + s8;
  _sma_idx = (_sma_idx + 1) & (SMA_WINDOW - 1);
}

void CPUUsageTracker::begin() {
  s_idle_handle = xTaskGetIdleTaskHandleForCPU(0);
  esp_register_freertos_tick_hook_for_cpu(s_tick_hook, 0);

  esp_timer_create_args_t args = {
    .callback        = s_sample_cb,
    .arg             = this,
    .dispatch_method = ESP_TIMER_TASK,
    .name            = "cpu_sample"
  };
  esp_timer_create(&args, &_timer);
  esp_timer_start_periodic(_timer, 60000000ULL / SMA_WINDOW);  // SMA_WINDOW samples per minute
}

#endif
