/*
 * display_service.ino
 * Adapter für das OLED Display. Hier passiert ALLES was mit Zeichnen zu tun hat.
 */

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "header.h" // Um NVSAdapter::isAnyPumpScheduledAt etc. abzufragen

namespace DisplayAdapter {

  // =============================================
  // INTERNE HARDWARE-OBJEKTE
  // =============================================
  const int SCREEN_WIDTH  = 128;
  const int SCREEN_HEIGHT = 64;
  const int OLED_RESET    = -1;
  const uint8_t OLED_ADDRESS = 0x3C;
  
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

  // =============================================
  // INTERNE HILFSMETHODEN
  // =============================================
  
  // Zeichnet die 16 Pixel hohe Kopfzeile im oberen (gelben) Bereich des OLED
  void drawHeader(const String& text, bool inverted = false) {
    if (inverted) {
      display.fillRect(0, 0, SCREEN_WIDTH, 16, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.fillRect(0, 0, SCREEN_WIDTH, 16, SSD1306_BLACK); // Löscht alten Header
      display.setTextColor(SSD1306_WHITE);
    }

    display.setTextSize(2);
    display.setCursor(1, 1);
    
    // Max 12 Zeichen, damit es nicht über den Rand rutscht
    String shortened = text;
    if (shortened.length() > 12) shortened = shortened.substring(0, 12);
    
    display.print(shortened);
    display.setTextColor(SSD1306_WHITE); // Für den Rest des Displays zurücksetzen
  }

  // =============================================
  // ÖFFENTLICHE METHODEN (Das "Interface")
  // =============================================

  void init() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
      Serial.println("❌ FEHLER: OLED Display nicht gefunden!");
      while (true); // Stopp, da das Gerät ohne UI nutzlos ist
    }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.display();
    Serial.println("📺 DisplayAdapter initialisiert.");
  }

  // Schaltet das Display hart ab (für Deep Sleep)
  void turnOff() {
    display.clearDisplay();
    display.display();
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }

  // Zeichnet simple Meldungen (z.B. "SPEICHERN", "GOODBYE")
  void showMessage(const String& msg) {
    display.clearDisplay();
    drawHeader(msg);
    display.display();
  }

  // =============================================
  // MENÜ-ANSICHTEN (Die "Views")
  // =============================================

  // Startbildschirm (Idle)
  void drawIdle(int selectedPump, bool isPumpRunning, int runningPumpNr) {
    display.clearDisplay();

    // 1. Header zeichnen
    if (isPumpRunning) {
      // Wenn eine Pumpe läuft, lassen wir den Header im Sekundentakt blinken (invertieren)
      bool blink = (millis() / 1000) % 2 == 0;
      drawHeader("PUMPE " + String(runningPumpNr + 1), blink);
    } else {
      drawHeader("UEBERSICHT");
    }

    // 2. Das 4x24 Raster zeichnen
    const int blockWidth  = 5; 
    const int blockHeight = 5;
    const int xOffset     = 8; 
    const int yOffset     = 20; 
    const int lineSpacing = 12;

    for (uint8_t p = 0; p < 4; ++p) {
      int y = yOffset + p * lineSpacing;
      display.setTextSize(1);

      // Pumpennummer links (invertiert bei Auswahl)
      if (p == selectedPump) {
        display.fillRect(0, y - 1, 7, 9, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(1, y);
        display.print(p + 1);
        display.setTextColor(SSD1306_WHITE);
      } else {
        display.setCursor(1, y);
        display.print(p + 1);
      }

      // Stunden-Balken abfragen (über unseren sauberen NVSAdapter!)
      for (uint8_t h = 0; h < 24; ++h) {
        int x = xOffset + h * blockWidth;
        
        // AUFRUF: Liegt eine Zeit > 0 vor?
        if (NVSAdapter::getWateringTime(p, h) > 0) {
          display.fillRect(x, y, blockWidth - 1, blockHeight, SSD1306_WHITE);
        } else {
          // Kleiner Punkt für "Stunde existiert, aber leer"
          display.drawPixel(x + 1, y + blockHeight / 2, SSD1306_WHITE);
        }
      }
    }
    display.display();
  }

  // =============================================
  // DAS HAUPTMENÜ
  // =============================================
  void drawMainMenu(int selectedIndex) {
    display.clearDisplay();
    drawHeader("HAUPTMENU");

    // Unsere Menü-Einträge
    const char* menuItems[] = {
      "Uhrzeit einstellen",
      "Pumpe 1 Config",
      "Pumpe 2 Config",
      "Pumpe 3 Config",
      "Pumpe 4 Config"
    };
    const int itemCount = 5;

    display.setTextSize(1);
    
    // Start-Position direkt unter dem gelben Header
    const int startY = 18; 
    const int lineSpacing = 9; // 8px Schrift + 1px Puffer

    for (int i = 0; i < itemCount; ++i) {
      int y = startY + (i * lineSpacing);

      if (i == selectedIndex) {
        // 1. Weißen Hintergrundbalken zeichnen
        // Wir ziehen den Balken über die volle Breite (SCREEN_WIDTH)
        display.fillRect(0, y - 1, SCREEN_WIDTH, lineSpacing, SSD1306_WHITE);
        
        // 2. Textfarbe auf Schwarz setzen (Invertierung)
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(2, y);
        display.print("> ");
        display.print(menuItems[i]);
        
        // 3. Textfarbe sofort wieder auf Weiß stellen für die nächsten Zeilen
        display.setTextColor(SSD1306_WHITE); 
      } else {
        display.setCursor(2, y);
        // "  " (zwei Leerzeichen) als Platzhalter, damit der Text bündig 
        // unter dem invertierten Eintrag steht.
        display.print("  "); 
        display.print(menuItems[i]);
      }
    }

    display.display();
  }

  // Uhrzeit einstellen
  void drawClockSetting(int hour, int minute, bool editingHours) {
    display.clearDisplay();
    drawHeader("UHRZEIT");

    display.setTextSize(3);
    display.setCursor(0, 30);

    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", hour, minute);
    display.print(buf);

    // Unterstrich für den aktuell fokussierten Wert
    const int underlineX = editingHours ? 0 : 54;
    display.fillRect(underlineX, 54, 33, 2, SSD1306_WHITE);

    display.display();
  }

  // Pumpen-Dauer für eine konkrete Stunde einstellen
  void drawPumpConfig(int pumpNum, int hour, int durationSeconds, bool isEditingDuration) {
    display.clearDisplay();
    drawHeader("PUMPE: " + String(pumpNum + 1));

    display.setTextSize(1);
    display.setCursor(0, 24);
    display.print("START (h):");
    display.setCursor(0, 48);
    display.print("DAUER (s):");

    char startBuf[3];
    char durBuf[4];
    snprintf(startBuf, sizeof(startBuf), "%02d", hour);
    snprintf(durBuf, sizeof(durBuf), "%03d", durationSeconds);

    display.setTextSize(2);
    display.setCursor(80, 20);
    display.print(startBuf);
    display.setCursor(80, 44);
    display.print(durBuf);

    // Unterstrich für Fokussierung
    int yUnderline = isEditingDuration ? 63 : 38;
    int width      = isEditingDuration ? 34 : 22;
    display.fillRect(80, yUnderline - 2, width, 2, SSD1306_WHITE);

    display.display();
  }

  // Bestätigungs-Dialog
  void drawConfirmDelete() {
    display.clearDisplay();
    drawHeader("LOESCHEN?");
    
    display.setTextSize(1);
    display.setCursor(0, 28);
    display.print("SET  = JA");
    display.setCursor(0, 48);
    display.print("BACK = NEIN");
    
    display.display();
  }

} // Ende Namespace