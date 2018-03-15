/*
 * INCUVERS OPEN INCUBATOR (IOI)
 *    Date:    March 2018
 *    Software version: 1.9
 *    Hardware version: 0.9.3
 *    http://spiderwort.bio/   
 *    
 *    Incuvers team:
 *        Dr. Sebastian Hadjiantoniou
 *        Tim Spekkens
 */

 /* Changelog
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
  * 1.4 - Rewrote the majority of the code for better stack efficiency and to allow for parallel operation.
  * 
  * 1.3 - Added the ability to save settings to the EEPROM.
  *     - Fixed error leading to temperature sensors becoming swapped.
  *     - Improved error condition shutdown handling.
  */

// Timing definitions
// Due to the long overflow and millis() reset, which would cause issues for our timing, we elect to reboot the incubator after one month of uptime.
#define RESET_AFTER_DELTA 2678400000

// Debugging definitions, remove definition to disable.  Supported debugging variants: DEBUG_GENERAL, DEBUG_SERIAL, DEBUG_EEPROM, DEBUG_UI, DEBUG_CO2, DEBUG_O2, DEBUG_TEMP, DEBUG_MEMORY
//define DEBUG_GENERAL true
//define DEBUG_SERIAL true
//define DEBUG_EEPROM true
//define DEBUG_UI true
//define DEBUG_CO2 true
//define DEBUG_O2 true
//define DEBUG_TEMP true
#define DEBUG_MEMORY true

// Hardwired settings
#define PINASSIGN_ONEWIRE_BUS 4
#define PINASSIGN_HEATCHAMBER 5
#define PINASSIGN_FAN 6
#define PINASSIGN_HEATDOOR 8

// Includes
// OneWire and DallasTemperature libraries used for reading the heat sensors
#include "OneWire.h"
#include "DallasTemperature.h"
// Serial library used for querying the CO2/O2 sensor
#include "SoftwareSerial.h"
// Wire and LiquidTWI2 libraries used for the LCD display and button interface
#include "Wire.h"
#include "LiquidTWI2.h"
// EEPROM library used for accessing settings
#include <EEPROM.h>  

// Incuvers modules 
#include "Incuvers_SerialSensorWrapper.h"
#include "Incuvers_Heat.h"
#include "Incuvers_CO2.h"
#include "Incuvers_O2.h"
#include "Incuvers_Settings.h"
#include "Incuvers_UI.h"

// Globals
IncuversSettingsHandler* iSettings;
IncuversHeatingSystem* iHeat;
IncuversCO2System* iCO2;
IncuversO2System* iO2;
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

  iCO2 = new IncuversCO2System();
  iSettings->AttachIncuversModule(iCO2);
  
  iO2 = new IncuversO2System();
  iSettings->AttachIncuversModule(iO2);

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
  
  // Give all the modules a chance to do some work
  iHeat->DoTick();
  iCO2->DoTick();
  iO2->DoMiniTick();
  iHeat->DoTick();
  iO2->DoTick();
  iCO2->DoMiniTick();
  iUI->DoTick(); 
}

