#include <Arduino.h>
#include "header.h"

namespace MenuManager {

  TaskHandle_t taskHandle;
  volatile bool _isRunning = false;

  bool isRunning() { return _isRunning; }
  void start() { if (!_isRunning) xTaskNotifyGive(taskHandle); }

  // Unsere Zustände
  enum MenuState {
    STATE_IDLE,
    STATE_MAIN_MENU,
    STATE_MANUAL_RUN, 
    STATE_SET_CLOCK,
    STATE_CONFIG_PUMP
  };
  MenuState currentState = STATE_IDLE;

  // Ein paar ganz wenige Variablen brauchen wir, um zu wissen "wo" wir im Menü stehen:
  int selectedPump = 0; // Für Idle und Config
  int mainMenuIndex = 0; 
  int configHour = 0;   // Welche Stunde schauen wir uns im Config-Menü gerade an?
  bool isEditingDuration = false;
  
  int editHour = 0;
  int editMinute = 0;
  bool isEditingMinute = false;
  // ==========================================
  // HANDLER-FUNKTIONEN (Klein & Spezialisiert!)
  // ==========================================

  void handleSetClock(int rotary, bool btnSet, bool btnBack, bool &needsRedraw) {
    if (btnBack) {
      currentState = STATE_MAIN_MENU;
      return;
    }

    if (btnSet) {
      isEditingMinute = !isEditingMinute; // Umschalten zwischen Stunde und Minute
    }

    if (rotary != 0) {
      if (!isEditingMinute) {
        editHour += rotary;
        if (editHour > 23) editHour = 0;
        if (editHour < 0) editHour = 23;
      } else {
        editMinute += rotary;
        if (editMinute > 59) editMinute = 0;
        if (editMinute < 0) editMinute = 59;
      }
      
      // Bei jeder Drehung die RTC direkt updaten!
      RTCAdapter::setTime(editHour, editMinute);
    }

    if (needsRedraw) {
      // !isEditingMinute weil wir den Boolean "editingHours" übergeben müssen
      DisplayAdapter::drawClockSetting(editHour, editMinute, !isEditingMinute);
    }
  }

  void handleIdle(int rotary, bool btnSet, bool btnMenu, bool &needsRedraw) {
    // 1. (scroll) -> selektiert pumpe 1-4
    if (rotary != 0) {
      selectedPump += rotary;
      if (selectedPump > 3) selectedPump = 0;
      if (selectedPump < 0) selectedPump = 3;
    }

    // 2. (menu-taste) -> mainMenu
    if (btnMenu) {
      currentState = STATE_MAIN_MENU;
      mainMenuIndex = 0; // Cursor zurücksetzen
      return; // WICHTIG: Funktion sofort verlassen, damit nichts anderes mehr passiert!
    }

    // 3. (set) -> startet diese pumpe (directRunPumpMenu)
    if (btnSet) {
      PumpAdapter::on(selectedPump);
      currentState = STATE_MANUAL_RUN;
      return;
    }

    if (needsRedraw) {
      DisplayAdapter::drawIdle(selectedPump, false, 0);
    }
  }

  void handleMainMenu(int rotary, bool btnSet, bool btnBack, bool &needsRedraw) {
    const int MENU_ITEMS_COUNT = 5; 

    // 1. (back) -> zurück zum idleMenu
    if (btnBack) {
      currentState = STATE_IDLE;
      return;
    }

    // 2. (scroll) -> Menü selektieren
    if (rotary != 0) {
      mainMenuIndex += rotary;
      // Durchscrollen (Wrap-around)
      if (mainMenuIndex >= MENU_ITEMS_COUNT) mainMenuIndex = 0;
      if (mainMenuIndex < 0) mainMenuIndex = MENU_ITEMS_COUNT - 1;
    }

    // 3. (set) -> das selektierte Menü öffnen
    if (btnSet) {
      if (mainMenuIndex == 0) {
        // ➔ Wechsel zu "Uhrzeit einstellen"
        currentState = STATE_SET_CLOCK;
        
        editHour = RTCAdapter::getHour();
        editMinute = RTCAdapter::getMinute();
        isEditingMinute = false; // Wir fangen immer bei der Stunde an
        
      } else {
        // ➔ Wechsel zu "Pumpen Config"
        selectedPump = mainMenuIndex - 1; // Index 1-4 wird zu Pumpe 0-3
        
        // Startwerte für das Pumpen-Menü vorbereiten
        configHour = 0;             // Immer bei 00:00 Uhr starten
        isEditingDuration = false;  // Fokus liegt zuerst auf der Stunde
        
        currentState = STATE_CONFIG_PUMP;
      }
      return; // Zustandswechsel ist passiert -> Sofort raus hier!
    }

    // 4. Zeichnen
    if (needsRedraw) {
      // Diese Methode müssten wir dem DisplayAdapter noch beibringen ;)
      // Sie würde z.B. ein Array von Strings nehmen und den Cursor (>) zeichnen.
      DisplayAdapter::drawMainMenu(mainMenuIndex);
    }
  }

  void handleManualRun(int rotary, bool btnSet, bool btnBack, bool btnMenu, bool &needsRedraw) {
    // (egal welche taste) -> pumpe stoppen und zum idleMenu
    if (rotary != 0 || btnSet || btnBack || btnMenu) {
      PumpAdapter::offAll();
      currentState = STATE_IDLE; // Zustandswechsel zurück!
      return;
    }

    if (needsRedraw) {
      // Hier rufen wir z.B. drawIdle auf, sagen ihm aber "Pumpe läuft!" (fürs Blinken)
      DisplayAdapter::drawIdle(selectedPump, true, selectedPump);
    }
  }

  void handleConfigPump(int rotary, bool btnSet, bool btnBack, bool &needsRedraw) {
    // 1. (back) -> zurück zum mainMenu
    if (btnBack) {
      currentState = STATE_MAIN_MENU;
      isEditingDuration = false; // Reset für den nächsten Besuch
      return;
    }

    // 2. (set) -> wechsel von stunde auf dauer und zurück
    if (btnSet) {
      isEditingDuration = !isEditingDuration; // Einfach umdrehen (Toggle)
    }

    // 3. (scroll) -> aktuelle Auswahl verändern
    if (rotary != 0) {
      if (!isEditingDuration) {
        // Wir verändern die Stunde
        configHour += rotary;
        if (configHour > 23) configHour = 0;
        if (configHour < 0) configHour = 23;
      } 
      else {
        // Wir verändern die DAUER. Wir lesen den aktuellen RAM-Wert, 
        // addieren den Drehimpuls und schreiben ihn direkt zurück in den RAM!
        int currentDuration = NVSAdapter::getWateringTime(selectedPump, configHour);
        
        // Kleiner Trick: Bei < 15 sek machen wir 1er Schritte, danach z.B. 5er Schritte
        int step = (currentDuration < 15) ? 1 : 5; 
        
        int newDuration = currentDuration + (rotary * step);
        if (newDuration < 0) newDuration = 0;
        if (newDuration > 300) newDuration = 300; // Max 5 Minuten

        // DIREKTES SPEICHERN IM RAM! (Wird beim Beenden des Menüs in den Flash geflusht)
        NVSAdapter::setWateringTime(selectedPump, configHour, newDuration);
      }
    }

    // 4. Zeichnen
    if (needsRedraw) {
      // Wir holen uns für die Anzeige einfach den tagesaktuellen Wert aus dem RAM
      int currentDuration = NVSAdapter::getWateringTime(selectedPump, configHour);
      DisplayAdapter::drawPumpConfig(selectedPump, configHour, currentDuration, isEditingDuration);
    }
  }

  // ==========================================
  // DER HAUPT-TASK
  // ==========================================
  void menuTask(void *param) {
    while (true) {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      _isRunning = true;
      currentState = STATE_IDLE; 
      bool needsRedraw = true;

      while (_isRunning) {
        int rotary   = InputAdapter::consumeRotary();
        bool btnSet  = InputAdapter::consumeBtnSet();
        bool btnBack = InputAdapter::consumeBtnBack();
        bool btnMenu = InputAdapter::consumeBtnMenu();

        if (rotary != 0 || btnSet || btnBack || btnMenu) needsRedraw = true;

        // Wir merken uns, wo wir herkommen
        MenuState previousState = currentState;

        // DIE WEICHE
        switch (currentState) {
          case STATE_IDLE:         handleIdle(rotary, btnSet, btnMenu, needsRedraw); break;
          case STATE_MAIN_MENU:    handleMainMenu(rotary, btnSet, btnBack, needsRedraw); break;
          case STATE_MANUAL_RUN:   handleManualRun(rotary, btnSet, btnBack, btnMenu, needsRedraw); break;
          case STATE_CONFIG_PUMP:  handleConfigPump(rotary, btnSet, btnBack, needsRedraw); break;
          case STATE_SET_CLOCK:    handleSetClock(rotary, btnSet, btnBack, needsRedraw); break;
        }

        // REDRAW FIX: Wenn wir den Zustand gewechselt haben, MÜSSEN wir im nächsten Loop zeichnen!
        if (currentState != previousState) {
          needsRedraw = true;
          // (0=IDLE, 1=MAIN_MENU, 2=MANUAL, 3=SET_CLOCK, 4=CONFIG_PUMP)
          Serial.printf("🔄 Menü-Zustand gewechselt auf: %d\n", currentState);
        } else {
          needsRedraw = false; // Nur zurücksetzen, wenn wir im selben Menü geblieben sind
        }

        vTaskDelay(pdMS_TO_TICKS(50));
      }
      
      NVSAdapter::flush(); 
      DisplayAdapter::turnOff();
    }
  }

  void init() {
    xTaskCreate(menuTask, "MenuTask", 4096, nullptr, 1, &taskHandle);
  }

} // namespace Ende