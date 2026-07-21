#pragma once

// OLED MAIN MENU MODE
#define MAIN_TIMEOUT_SEC (900 * LOOPS_PER_SEC)  // 15 min

// OLED MODES
#define OLED_OFF 0
#define OLED_MAIN 1
#define OLED_POPUP 2
#define OLED_MINIMIZED 3
#define OLED_MODAL 4
#define OLED_PAUSE_LOGGER 5
#define OLED_MODE_NO_TIMEOUT 0xFFFF

#define OLED_HOLD_TIMEOUT 20

void initDisplay();
void updateOLED();
void showPopup(const char* text, const char* details, uint16_t timeout = 0xFFFF, uint8_t nextMode = OLED_MAIN);
void newPopupScreen(const char* text, const char* details, uint16_t timeout = 0xFFFF, uint8_t nextMode = OLED_MAIN);
void oledMain(uint32_t duration = MAIN_TIMEOUT_SEC);
void oledMinimized();
void oledBlank();
void oledPauseLogger();
void oledModal(const char* title);
void oledOn();
void oledOff();
bool modalEvent();
void clearModalEvent();

// getters
uint8_t getOledMode();