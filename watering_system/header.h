// RTCAdapter.h
#pragma once

// 1. Die Tasks (da du diese auch ausgelagert hast!)
namespace WateringManager {
  void init();
  void start();          
  bool isRunning();      
}

namespace MenuManager {
  void init();
  void start();
  bool isRunning();
}

// 2. Der RTC Adapter
namespace RTCAdapter {
  void init();
  int getHour();
  int getMinute();
  void setTime(int hour, int minute);
}

// 3. Der NVS (Speicher) Adapter
namespace NVSAdapter {
  void init();
  uint16_t getWateringTime(uint8_t pumpNr, uint8_t hour);
  bool isAnyPumpScheduledAt(uint8_t hour);
  void setWateringTime(uint8_t pumpNr, uint8_t hour, uint16_t durationSeconds);
  void clearPumpSchedule(uint8_t pumpNr);
  void flush();
}

// 4. Der Pumpen Adapter
namespace PumpAdapter {
  void init();
  void on(uint8_t pumpNr);
  void off(uint8_t pumpNr);
  void offAll(); 
}

// 5. Der Input Adapter
namespace InputAdapter {
  void init();
  int consumeRotary();
  bool consumeBtnSet();
  bool consumeBtnMenu();
  bool consumeBtnBack();
  uint32_t getIdleTimeMs(); // NEU: Sagt uns, wie lange niemand mehr was gedrückt hat
}

namespace DisplayAdapter {
  void init();
  void turnOff();
  void showMessage();
  void drawConfirmDelete();
  void drawPumpConfig();
  void drawClockSetting();
  void drawIdle();
}