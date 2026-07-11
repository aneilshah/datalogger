#pragma once

// OLED MAIN MENU MODE
#define MAIN_TIMEOUT_SEC (900 * LOOPS_PER_SEC)  // 15 min

// OLED MODES
#define OLED_OFF 0
#define OLED_MAIN 1
#define OLED_POPUP 2
#define OLED_MINIMIZED 3
#define OLED_LOW_POWER 4
#define OLED_HALF_POWER 5
#define OLED_MODE_NO_TIMEOUT 10

#define OLED_POWER_TIMEOUT 30

void initDisplay();
void displayText();
void displayPopupScreen(const char* text, const char* details);
void newPopupScreen(const char* text, const char* details);
void updatePopupScreen();
void oledMain(uint32_t duration = MAIN_TIMEOUT_SEC);
void oledMinimized();
void oledBlank();
void oledHalfPower();
void oledLowPower();
void oledOn();
void oledOff();

// getters
uint8_t getOledMode();