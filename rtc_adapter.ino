/*
 * Service-Modul für die externe Echtzeituhr (DS3231) via I2C
 */

#include <Wire.h>
#include <RTClib.h>

namespace RTCAdapter {
  
  // Das Hardware-Objekt für die externe RTC
  RTC_DS3231 rtc;

  // =============================================
  // INITIALISIERUNG
  // =============================================
  void init() {
    if (!rtc.begin()) {
      Serial.println("❌ FEHLER: Externe RTC (DS3231) nicht gefunden!");
      Serial.println("Bitte I2C-Verkabelung (SDA/SCL) prüfen.");
      // Wir stoppen das Programm hier nicht komplett, aber 
      // es wird eine Fehlermeldung ausgegeben, wenn die Kabel locker sind.
    } else {
      Serial.println("✅ Externe RTC erfolgreich initialisiert.");
      
      // Ein kleiner, aber wichtiger Check: Ist die Knopfzelle leer?
      if (rtc.lostPower()) {
        Serial.println("⚠️ RTC hat den Speicher verloren (Batterie leer/neu?). Setze Notfall-Zeit.");
        // Setzt die Uhr auf den Zeitpunkt, an dem das Programm kompiliert wurde
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); 
      }
    }
  }

  // =============================================
  // LESEN
  // =============================================
  int getHour() { 
    return rtc.now().hour(); 
  }

  int getMinute() { 
    return rtc.now().minute(); 
  }

  // =============================================
  // SCHREIBEN
  // =============================================
  void setTime(int hour, int minute) {
    DateTime now = rtc.now();
    
    // Datum lassen wir so wie es ist. Wir ändern nur die Stunde und Minute.
    // Die Sekunden setzen wir hart auf 0, damit die neue Zeit präzise startet.
    rtc.adjust(DateTime(now.year(), now.month(), now.day(), hour, minute, 0));
    
    Serial.printf("⏰ Uhrzeit manuell aktualisiert auf: %02d:%02d\n", hour, minute);
  }
}