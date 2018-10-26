/*
 * INCUVERS INCUBATOR 
 *    Date:    October 2018
 *    Software version: 1.11
 *    Hardware version: 1.0.0
 *    http://incuvers.com/   
 *    
 *    Incuvers team:
 *        Dr. Sebastian Hadjiantoniou
 *        Tim Spekkens
 */

 /* Changelog
  * 
  * 1.11 - General code clean up and housekeeping.
  *      - Switched serial sensors from streaming mode to on-demand polling.
  *      - Added support for Modbus-based or voltage-based luminox sensors.
  *      - Moved common environmental management code into its own class.
  *      - Moved to support Control Board 1.0.0 / ATMEGA 2560 controller.
  *      - Added some power management features to limit the current draw.
  *      
  * 1.10 - Added support for chamber lighting control.
  *      - Started making modules/skeletons to save on memory in Atmega328 implementations.
  * 
  * 1.9 - Moved key configuration elements to hardware-specific settings store.
  *     - Rewrote the serial sensor monitoring systems.
  *     - Subdivided code into several files in order to aid in maintenance.
  *     - Added selectable fan modes
  *     - improved the DEBUG options for smaller compiled results in production environments.
  *     - removed ability to import settings
  *     
  * 1.8 - Addition of support for Digital O2 sensors.
  *
  * 1.7 - Addition of O2 sensor and related hardware
  * 
  * 1.6 - Bug fixes pertaining to display, alarms, and menu functionality.
  * 
  * 1.5 - Rewriting of alarm handling code.
  *     - Addition of better sensor error detection and warning.
  * 
  * 1.4 - Rewrote the majority of the code for better stack efficiency and to allow for parallel operations.
  * 
  * 1.3 - Added the ability to save settings to the EEPROM.
  *     - Fixed error leading to temperature sensors becoming swapped.
  *     - Improved error condition shutdown handling.
  */

// Timing definitions
// Due to the long overflow and millis() reset, which would cause issues for our timing, we elect to reboot the incubator after one month of uptime.
#define RESET_AFTER_DELTA 2678400000

// Use the capabilities of the ATMEGA 2560 microcontroller (hardware serial for sensors, etc)
#define USE_2560 true

// Debugging definitions, comment out to disable
//#define DEBUG_GENERAL true
//#define DEBUG_SERIAL true
//#define DEBUG_EEPROM true
//#define DEBUG_UI true
#define DEBUG_EM true
//#define DEBUG_CO2 true
//#define DEBUG_O2 true
//#define DEBUG_TEMP true
//#define DEBUG_LIGHT true
#define DEBUG_MEMORY true

// Build/upload-time options - comment out unneeded modules in order to save program space.  Please only ensure only one O2 module is included at any given time.
#define INCLUDE_O2_SERIAL true
//#define INCLUDE_O2_MODBUS true
//#define INCLUDE_O2_ANALOG true
#define INCLUDE_CO2 true
#define INCLUDE_TEMP true
#define INCLUDE_LIGHT true
#define INCLUDE_PILINK true

// Hardwired settings
#define PINASSIGN_ONEWIRE_BUS 4
#define PINASSIGN_HEATDOOR 8
#define PINASSIGN_HEATCHAMBER 9
#define PINASSIGN_FAN 10

// Includes
// OneWire and DallasTemperature libraries used for reading the heat sensors
#include "OneWire.h"
#include "DallasTemperature.h"
#ifndef USE_2560 
  // Serial library used for querying the CO2/O2 sensor (ATMEGA 328 compatibility)
  #include "SoftwareSerial.h"
#endif
// Wire and LiquidTWI2 libraries used for the LCD display and button interface
#include "Wire.h"
#include "LiquidTWI2.h"
// EEPROM library used for accessing settings
#include <EEPROM.h>  

// Incuvers modules 
#include "Incuvers_Common.h"
#include "Incuvers_EnvironmentalManager.h"

#ifdef INCLUDE_O2_MODBUS
 #include <ModbusMaster.h>
 #include "SenseWrap_Modbus.h"
#endif
#if defined(INCLUDE_O2_SERIAL) || defined(INCLUDE_CO2)
 #include "SenseWrap_Serial.h"
#endif

#ifdef INCLUDE_O2_MODBUS
 #include "Env_O2_Luminox_Modbus.h"
#endif
#ifdef INCLUDE_O2_SERIAL
 #include "Env_O2_Luminox_Serial.h"
#endif
#ifdef INCLUDE_O2_ANALOG
 #include "Env_O2_Luminox_Analog.h"
#endif
#include "Env_Heat.h"
#include "Env_CO2_COZIR.h"
#include "Opt_Light.h"
#include "Incuvers_Settings.h"
#include "Opt_PiLink.h"
#include "Incuvers_UI.h"

// Globals
IncuversSettingsHandler* iSettings;
IncuversHeatingSystem* iHeat;
IncuversLightingSystem* iLight;
IncuversCO2System* iCO2;
IncuversO2System* iO2;
IncuversPiLink* iPi;
IncuversUI* iUI;

void setup() {
  // Start serial port
  Serial.begin(9600);

  iUI = new IncuversUI();
  iUI->SetupUI();
  iUI->DisplayStartup();
  
  iSettings = new IncuversSettingsHandler();
  int runMode = 0; // 0 = uninitialized
                   // 1 = saved settings
                   // 2 = default settings
  runMode = iSettings->PerformLoadSettings();
  iSettings->CheckSettings();

  if (runMode == 0) {
    iUI->WarnOfMissingHardwareSettings();
  }

  iHeat = new IncuversHeatingSystem();
  iSettings->AttachIncuversModule(iHeat);

  iLight = new IncuversLightingSystem();
  iSettings->AttachIncuversModule(iLight);
  
  iCO2 = new IncuversCO2System();
  iSettings->AttachIncuversModule(iCO2);
  
  iO2 = new IncuversO2System();
  iSettings->AttachIncuversModule(iO2);

  iPi = new IncuversPiLink();
  iPi->SetupPiLink(iSettings);
  
  iUI->AttachSettings(iSettings);
  iUI->DisplayRunMode(runMode); 
  if (runMode == 2) {
    // Don't have the info we need, load default settings and go into setup
    iUI->EnterSetupMode();
  }
}

void loop() {
  unsigned long nowTime = millis();

  if (nowTime > RESET_AFTER_DELTA) {
    #ifdef DEBUG_GENERAL
      Serial.println(F("loop() call detected the Arduino has been on for a month, need to reset"));
    #endif
    asm volatile ("  jmp 0"); 
  }
  
  // Give all the modules a chance to do some work - While we used to use mini-ticks to support multiple Software-emulated serial connections, switching away from streaming mode
  // on the serial sensors negates the need to employ these extra steps.
  iHeat->DoTick();
  iCO2->DoTick();
  iO2->DoTick();
  iHeat->DoQuickTick();
  iLight->DoTick();
  iUI->DoTick(); 
  iPi->DoTick();
}

