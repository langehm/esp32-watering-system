#include <Arduino.h>
#include "Header.h"

namespace WateringManager {

  TaskHandle_t taskHandle;
  volatile bool _isRunning = false;

  bool isRunning() { return _isRunning; }
  
  void start() { 
    if (!_isRunning) xTaskNotifyGive(taskHandle); 
  }

  void wateringTask(void *param) {
    while (true) {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      _isRunning = true;
      
      DisplayAdapter::showMessage("WATERING...");
      
      int currentHour = RTCAdapter::getHour();
      
      for (uint8_t p = 0; p < 4; ++p) {
        uint16_t duration = NVSAdapter::getWateringTime(p, currentHour);
        if (duration > 0) {
          Serial.printf("💦 Pumpe %d läuft für %d Sekunden...\n", p+1, duration);
          PumpAdapter::on(p);
          vTaskDelay(pdMS_TO_TICKS(duration * 1000UL)); 
          PumpAdapter::off(p);
        }
      }
      
      _isRunning = false;
    }
  }

  void init() {
    xTaskCreate(wateringTask, "WateringTask", 4096, nullptr, 1, &taskHandle);
  }

} // namespace Ende