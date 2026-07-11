// Button.cpp
#include "global.h"

#include "oled.h"
#include "wifiFunc.h"

void processButton() {
  Serial.println("Button Event");
  displayPopupScreen("BUTTON PRESSED", "Show Main Menu");
  delay(1000);
  if (wifiRadioOn()) {
    newPopupScreen("Turn OFF Wifi", "");
    disconnectWifi();
    oledOff();
  } else {
    newPopupScreen("Turn ON Wifi", "");
    connectWifi();
    oledOn();
  }
  delay(1000);
  oledMain(MAIN_TIMEOUT_SEC);
}