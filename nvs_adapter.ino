/*
 * Persistenter Speicher mit RAM-Caching und Dirty-Flag (Write-Back)
 */
#include <Preferences.h>

namespace NVSAdapter {
  Preferences preferences;

  // Unser superschneller Arbeitsspeicher
  uint16_t schedule[4][24] = {0}; 

  // Das magische Flag: True, wenn im RAM etwas geändert wurde, was noch nicht im Flash ist.
  volatile bool isDirty = false; 

  // =============================================
  // INITIALISIERUNG (Einmal beim Start)
  // =============================================
  void init() {
    preferences.begin("watering", true); // true = ReadOnly
    
    // Wir lesen den Blob in einem einzigen, extrem schnellen Rutsch
    size_t bytesRead = preferences.getBytes("schedBlob", schedule, sizeof(schedule));
    preferences.end();

    if (bytesRead > 0) {
      Serial.println("💾 Plan erfolgreich aus dem Flash geladen.");
    } else {
      Serial.println("ℹ️ Kein Plan gefunden. Array ist leer (0).");
    }
    isDirty = false;
  }

  // =============================================
  // LESEN (Nanosekunden aus dem RAM)
  // =============================================

  uint16_t getWateringTime(uint8_t pumpNr, uint8_t hour) {
    if (pumpNr > 3 || hour > 23) return 0;
    return schedule[pumpNr][hour];
  }

  bool isAnyPumpScheduledAt(uint8_t hour) {
    if (hour > 23) return false;
    for (uint8_t p = 0; p < 4; ++p) {
      if (schedule[p][hour] > 0) return true;
    }
    return false;
  }

  // =============================================
  // SCHREIBEN (Nur in den RAM, markiert als "Dirty")
  // =============================================

  void setWateringTime(uint8_t pumpNr, uint8_t hour, uint16_t durationSeconds) {
    if (pumpNr > 3 || hour > 23) return;

    // Hat sich der Wert wirklich geändert?
    if (schedule[pumpNr][hour] != durationSeconds) {
      schedule[pumpNr][hour] = durationSeconds; // Nur im RAM ändern!
      isDirty = true;                           // Merken, dass wir speichern müssen
      Serial.printf("✏️ Geändert im RAM: Pumpe %d, Stunde %d -> %d sek\n", pumpNr + 1, hour, durationSeconds);
    }
  }

  void clearPumpSchedule(uint8_t pumpNr) {
    if (pumpNr > 3) return;
    
    for (uint8_t h = 0; h < 24; ++h) {
      if (schedule[pumpNr][h] > 0) {
        schedule[pumpNr][h] = 0;
        isDirty = true;
      }
    }
    if (isDirty) Serial.printf("🗑️ Plan für Pumpe %d im RAM gelöscht.\n", pumpNr + 1);
  }

  // =============================================
  // FLUSH (Der tatsächliche Flash-Schreibvorgang)
  // =============================================

  // Wird beim Verlassen des Menüs oder vor dem Deep Sleep aufgerufen!
  void flush() {
    if (!isDirty) {
      // Nichts hat sich geändert, wir sparen uns den Flash-Zyklus!
      return; 
    }

    preferences.begin("watering", false); // false = Schreibzugriff
    preferences.putBytes("schedBlob", schedule, sizeof(schedule));
    preferences.end();

    isDirty = false; // Zurücksetzen
    Serial.println("💾 FLUSH: Änderungen wurden dauerhaft im Flash gespeichert!");
  }
}