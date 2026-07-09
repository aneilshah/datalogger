#pragma once

void initDisplay();
void displayText();
void displayPopupScreen(const char* text, const char* details);
void newPopupScreen(const char* text, const char* details);
void updatePopupScreen();
void oledMain(uint32_t duration);
void oledMinimized();
void oledOff();

#define OLED_OFF 0
#define OLED_MAIN 1
#define OLED_POPUP 2
#define OLED_MINIMIZED 3

#define OLED_MODE_NO_TIMEOUT 0
