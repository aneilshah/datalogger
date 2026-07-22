#pragma once

// OLED MAIN MENU MODE
#define MAIN_TIMEOUT_SEC (900 * LOOPS_PER_SEC)  // 15 min

// OLED MODES
#define OLED_OFF          0
#define OLED_MAIN         1
#define OLED_POPUP        2
#define OLED_MINIMIZED    3
#define OLED_MODAL        4
#define OLED_LOGGING      5

// Timeouts
#define OLED_HOLD_TIMEOUT 20

void initDisplay();
void updateOLED();
void showPopup(const char* text, const char* details);
void newPopupScreen(const char* text, const char* details);
bool modalEvent();
void clearModalEvent();

// Oled Screens
void oledMain();
void oledMinimized();
void oledBlank();
void oledLogging();
void oledModal(const char* title);
void oledOn();
void oledOff();

// getters
uint8_t getOledMode();