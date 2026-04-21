/*
 * watering_system.ino - V7
 * Architektur-Update: Input-System in InputAdapter ausgelagert.
 * Hauptdatei ist nun eine reine Kommandozentrale.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "driver/rtc_io.h"
#include "Header.h" // WICHTIG: Hier stecken alle Adapter-Vordeklarationen drin!

const uint8_t PIN_SDA = 15;
const uint8_t PIN_SCL = 16;
const unsigned long INACTIVITY_TIMEOUT_MS = 30000UL;

// =============================================
// TASK: Der Boss (Power & Timing Manager)
// =============================================
void powerManagerTask(void *param) {
  static int lastWateredHour = -1;

  esp_sleep_wakeup_cause_t wakeReason = esp_sleep_get_wakeup_cause();
  if (wakeReason == ESP_SLEEP_WAKEUP_EXT0) {
    MenuManager::start();
  } else if (wakeReason == ESP_SLEEP_WAKEUP_TIMER) {
    lastWateredHour = RTCAdapter::getHour(); 
    WateringManager::start();
  }

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(250));

    // Menü starten per Knopfdruck
    if (!MenuManager::isRunning() && InputAdapter::consumeBtnMenu()) {
      MenuManager::start();
    }

    int currentHour = RTCAdapter::getHour();
    int currentMinute = RTCAdapter::getMinute();
    bool needToWater = NVSAdapter::isAnyPumpScheduledAt(currentHour);

    if (needToWater && (currentMinute >= 5) && (currentHour != lastWateredHour) && !WateringManager::isRunning()) {
      lastWateredHour = currentHour;
      WateringManager::start();
    }

    // Deep Sleep Logik
if (!WateringManager::isRunning() && (InputAdapter::getIdleTimeMs() > INACTIVITY_TIMEOUT_MS)) { 
      Serial.println("😴 30s Inaktivität. Gehe in Deep Sleep...");
      
      // 1. Alles speichern, was noch im RAM liegt!
      NVSAdapter::flush();
      
      // 2. Licht aus!
      DisplayAdapter::turnOff();
      
      // 3. Wecker stellen (Wie viele Minuten bis zur nächsten xx:05 Uhrzeit?)
      int currentMinute = RTCAdapter::getMinute();
      int minutesToNextTarget = (60 - currentMinute + 5) % 60;
      if (minutesToNextTarget == 0) minutesToNextTarget = 60; 
      
      uint64_t sleepSeconds = minutesToNextTarget * 60ULL;

      // 4. Wakeup-Pins konfigurieren (Menü-Button auf Pin 18 zieht auf HIGH)
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_18, 1); 
      rtc_gpio_pullup_dis(GPIO_NUM_18);
      rtc_gpio_pulldown_en(GPIO_NUM_18);

      // 5. Timer konfigurieren
      esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000ULL);

      Serial.printf("💤 Schlafe für %llu Sekunden...\n", sleepSeconds);
      esp_deep_sleep_start();
    }
  }
}

// =============================================
// SETUP
// =============================================
void setup() {
  Serial.begin(115200);
  delay(1000); 

  Wire.begin(PIN_SDA, PIN_SCL); 

  // Hardware & Speicher laden
  DisplayAdapter::init();
  PumpAdapter::init();
  RTCAdapter::init(); 
  NVSAdapter::init();

  // 3. Main Tasks anlegen
  InputAdapter::init();
  MenuManager::init();
  WateringManager::init();
  xTaskCreate(powerManagerTask, "PowerTask", 2048, nullptr, 2, nullptr);
}

void loop() { 
  vTaskDelete(NULL); 
}