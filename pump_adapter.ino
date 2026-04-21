/*
 * pump_service.ino
 * Steuerung der Relais/Pumpen
 */

#include <Arduino.h>

namespace PumpAdapter {
  
  // Die Pins leben jetzt nur noch hier!
  const uint8_t PIN_BASE = 10; 
  const uint8_t PUMP_COUNT = 4;

  void init() {
    for (uint8_t i = 0; i < PUMP_COUNT; ++i) {
      pinMode(PIN_BASE + i, OUTPUT);
      digitalWrite(PIN_BASE + i, LOW); // Sicherstellen, dass sie aus sind
    }
    Serial.println("⚙️ PumpAdapter initialisiert.");
  }

  void on(uint8_t pumpNr) {
    if (pumpNr < PUMP_COUNT) {
      digitalWrite(PIN_BASE + pumpNr, HIGH);
    }
  }

  void off(uint8_t pumpNr) {
    if (pumpNr < PUMP_COUNT) {
      digitalWrite(PIN_BASE + pumpNr, LOW);
    }
  }

  void offAll() {
    for (uint8_t i = 0; i < PUMP_COUNT; ++i) {
      digitalWrite(PIN_BASE + i, LOW);
    }
  }

} // namespace Ende