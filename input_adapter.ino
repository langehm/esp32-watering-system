/*
 * input_service.ino
 * Adapter für Rotary Encoder und Buttons
 */

#include <Arduino.h>

namespace InputAdapter {

  // Die Pins leben jetzt exklusiv hier
  const uint8_t PIN_BTN_MENU = 18;
  const uint8_t PIN_BTN_BACK = 17;
  const uint8_t PIN_CLK      = 46; 
  const uint8_t PIN_DT       = 3;  
  const uint8_t PIN_BTN_SET  = 8;  

  // Interne Status-Variablen (sind durch den Namespace von außen unsichtbar!)
  volatile int rotaryDelta = 0;
  volatile bool flagBtnSet = false;
  volatile bool flagBtnMenu = false;
  volatile bool flagBtnBack = false;
  
  // Unser interner Aktivitäts-Timer
  volatile uint32_t lastActivityMs = 0;

  // =============================================
  // ÖFFENTLICHE METHODEN (Das "Interface")
  // =============================================
  
  int consumeRotary() { int val = rotaryDelta; rotaryDelta = 0; return val; }
  bool consumeBtnSet()  { if(flagBtnSet)  { flagBtnSet = false;  return true; } return false; }
  bool consumeBtnMenu() { if(flagBtnMenu) { flagBtnMenu = false; return true; } return false; }
  bool consumeBtnBack() { if(flagBtnBack) { flagBtnBack = false; return true; } return false; }

  // Sehr praktisch für den PowerManager!
  uint32_t getIdleTimeMs() {
    return millis() - lastActivityMs;
  }

  // =============================================
  // INTERNER TASK
  // =============================================
  void inputTask(void *param) {
    int lastClkState = digitalRead(PIN_CLK);
    bool lastSetState = HIGH;
    bool lastMenuState = LOW;
    bool lastBackState = LOW;

    while (true) {
      int clkState = digitalRead(PIN_CLK);
      if (clkState != lastClkState && clkState == LOW) {
        int dtState = digitalRead(PIN_DT);
        rotaryDelta += (dtState != clkState) ? 1 : -1;
        lastActivityMs = millis();
      }
      lastClkState = clkState;

      bool setState = digitalRead(PIN_BTN_SET);
      if (setState == LOW && lastSetState == HIGH) { flagBtnSet = true; lastActivityMs = millis(); }
      lastSetState = setState;

      bool menuState = digitalRead(PIN_BTN_MENU);
      if (menuState == HIGH && lastMenuState == LOW) { flagBtnMenu = true; lastActivityMs = millis(); }
      lastMenuState = menuState;

      bool backState = digitalRead(PIN_BTN_BACK);
      if (backState == HIGH && lastBackState == LOW) { flagBtnBack = true; lastActivityMs = millis(); }
      lastBackState = backState;

      vTaskDelay(pdMS_TO_TICKS(5));
    }
  }

  // =============================================
  // INITIALISIERUNG
  // =============================================
  void init() {
    pinMode(PIN_BTN_MENU, INPUT_PULLDOWN);
    pinMode(PIN_BTN_BACK, INPUT_PULLDOWN);
    pinMode(PIN_CLK, INPUT);
    pinMode(PIN_DT, INPUT);
    pinMode(PIN_BTN_SET, INPUT_PULLUP);

    lastActivityMs = millis();

    // Der Adapter startet seinen Hintergrund-Task ganz autark!
    xTaskCreate(inputTask, "InputTask", 2048, nullptr, 3, nullptr);
    Serial.println("🕹️ InputAdapter initialisiert.");
  }

} // Ende Namespace