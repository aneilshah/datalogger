// global.h
#pragma once

#include <Arduino.h>

class Diag;   // forward declaration

// external vaiables
extern int ADC1_COUNT;
//extern float ADC1_VOLT; 
extern const char* CONN_STATUS;
extern uint32_t LOOP_COUNT;
extern uint32_t LOOP_TIME;
extern int WIFI_ERR;
extern const char APP_VERSION[];     // (don’t put PROGMEM on the extern)

// Configurations

//---------------------------------
#define TEST_MODE 1
//---------------------------------

// Logging /Debug
#define VERBOSE 0
#define LOG_TIME 0  // serial log for execution times
#define ALLOW_WEBPAGE_POLLING 0

// Constants
#define OFF 0
#define ON 1
#define FALSE 0
#define TRUE 1

#define SENSOR_AMP_PER_VOLT 10
#define LOOPS_PER_SEC 10


// PIN MAPPINGS
#define ADC1_PIN 35
#define BTN_PIN 0
#define D1_PIN 0
//#define ISR_PIN 35
#define LED_PIN 25 // OLD Board 25, new board 35
#define TS1_PIN 14  // Touch Switch
#define ISR_PIN 35

// Computed Values (DO NOT EDIT)
#define PROD_MODE (TEST_MODE == 0)

// Header Constants
#define LOGGER_MAGIC 0x12345678
#define LOGGER_VERSION 1

// Needs to be last
#include "utils.h"
