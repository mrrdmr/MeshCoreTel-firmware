#pragma once
#ifdef ESP32

#include <stdint.h>
#include "esp_attr.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class CPUUsageTracker {
public:
  void begin();

  float getLoadAvg1()   const { return _avg1; }
  float getLoadAvg5()   const { return _avg5; }
  float getLoadAvg15()  const { return _avg15; }

private:
  static constexpr float DECAY1  = 0.920044415f;  // exp(-5/60)
  static constexpr float DECAY5  = 0.983471454f;  // exp(-5/300)
  static constexpr float DECAY15 = 0.994459848f;  // exp(-5/900)

  static volatile uint32_t s_idle_ticks;
  static volatile uint32_t s_busy_ticks;
  static TaskHandle_t      s_idle_handle;

  static void IRAM_ATTR s_tick_hook();
  static void           s_sample_cb(void* arg);

  uint32_t _last_idle   = 0;
  uint32_t _last_busy   = 0;
  float _avg1           = 0.0f;
  float _avg5           = 0.0f;
  float _avg15          = 0.0f;
  esp_timer_handle_t _timer = nullptr;

  void _onSample();
};

#endif
