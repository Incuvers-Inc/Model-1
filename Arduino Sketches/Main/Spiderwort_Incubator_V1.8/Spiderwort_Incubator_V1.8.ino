/*
 * SPIDERWORT OPEN CO2 INCUBATOR (SOCI)
 *    Date:    Feb 2018
 *    Version: 1.8
 *    http://spiderwort.bio/   
 */

 /* Changelog
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

// Defined constants
#define SETTINGS_IDENT_14 14
#define SETTINGS_IDENT 17
#define SETTINGS_ADDRS 64
#define MENU_UI_LOAD_DELAY 75

// Debugging definitions, remove definition to disable.  Supported debugging variants: DEBIG_GENERAL, DEBUG_CO2, DEBUG_O2, DEBUG_TEMP
#define DEBUG_GENERAL true
#define DEBUG_CO2 true
#define DEBUG_O2 true
#define DEBUG_TEMP true

// default settings
#define TEMPERATURE_MIN 5.0
#define TEMPERATURE_MAX 75.0
#define TEMPERATURE_DEF 37.0
#define TEMPERATURE_STEP_THRESH 0.998
#define TEMPERATURE_STEP_LEN 2000
#define CO2_MIN 0.5
#define CO2_MAX 25.0
#define CO2_DEF 5.0
#define CO2_STEP_THRESH 0.7
#define CO2_MULTIPLIER 10.0
#define CO2_DELTA_JUMP 3000
#define CO2_DELTA_STEPPING 250
#define CO2_BLEEDTIME_JUMP 5000
#define CO2_BLEEDTIME_STEPPING 5000
#define OO_MIN 0.5
#define OO_MAX 21.0
#define OO_DEF 10.0
#define OO_STEP_THRESH 1.3
#define N_DELTA_JUMP 10000
#define N_DELTA_STEPPING 250
#define N_BLEEDTIME_JUMP 2500
#define N_BLEEDTIME_STEPPING 5000

#define ALARM_THRESH 1.10
#define ALARM_TEMP_ON_PERIOD 1800000
#define ALARM_CO2_OPEN_PERIOD 600000
#define ALARM_CO2_READING_PERIOD 1000
#define ALARM_O2_READING_PERIOD 1000

// Pin assignment constants
#define PINASSIGN_CO2SENSOR_RX 2
#define PINASSIGN_CO2SENSOR_TX 11 // I'm not sure if this pin exists in our implementation, but we'll try it like this as we aren't trying to control the device
#define PINASSIGN_ONEWIRE_BUS 4
#define PINASSIGN_HEATCHAMBER 5
#define PINASSIGN_FAN 6
#define PINASSIGN_CO2 10
#define PINASSIGN_HEATDOOR 8
#define PINASSIGN_O2SENSOR_RX 3
#define PINASSIGN_O2SENSOR_TX 12 // I'm not sure if this pin exists in our implementation, but we'll try it like this as we aren't trying to control the sensor
#define PINASSIGN_NRELAY 7
/*  ATMEGA328 Pins In Use:
 *    D2 - Rx for CO2 sensor
 *    D3 - Tx for CO2 sensor
 *    D4 - DS18B20 temperature sensors
 *    D5 - Chamber Heater relay
 *    D6 - Fan relay
 *    D7 - CO2 Solenoid relay
 *    D8 - Door Heater relay
 *    D9 - Unused
 *    D10 - Ethernet 
 *    A4 - SDA (I2C) - Comms with MCP23017 I/O port expander  
 *    A5 - SCL (I2C) - Comms with MCP23017 I/O port expander 
 */

// Includes
// OneWire and DallasTemperature libraries used for reading the heat sensors
#include "OneWire.h"
#include "DallasTemperature.h"
// Serial library used for querying the CO2/O2 sensor
#include "SoftwareSerial.h"
// Wire and LiquidTWI2 libraries used for the LCD display and button interface
#include "Wire.h"
#include "LiquidTWI2.h"
// EEPROM library used for storing settings between sessions
#include <EEPROM.h>  

// Structures
struct SettingsStruct {
  byte ident;
  // Temp settings
  bool heatEnable;
  float heatSetPoint;
  byte sensorAddrDoorTemp[8];
  byte sensorAddrChamberTemp[8];
  // CO2 settings
  bool CO2Enable;
  float CO2SetPoint;
  // O2 settings
  bool O2Enable;
  float O2SetPoint;
};

struct SettingsStruct_14 {
  byte ident;
  // Temp settings
  bool heatEnable;
  float heatSetPoint;
  byte sensorAddrDoorTemp[8];
  byte sensorAddrChamberTemp[8];
  // CO2 settings
  bool CO2Enable;
  float CO2SetPoint;
};

struct StatusStruct {
  // General
  unsigned long LastLoopStartTime;
  int personalityCount;
  // Heat Status
  unsigned long DoorCheckpoint;     // Time to turn off door heater
  unsigned long ChamberCheckpoint;  // Time to turn off chamber heater
  float TempDoor;
  float TempChamber;
  boolean DoorHeatOn;
  boolean DoorHeatStepping;
  boolean ChamberHeatOn;
  boolean ChamberHeatStepping;
  // CO2
  unsigned long CO2Checkpoint;
  unsigned long CO2Actionpoint;
  unsigned long CO2Startpoint;
  float CO2Reading;
  boolean CO2On;
  boolean CO2Stepping;
  boolean CO2Started;
  // O2
  unsigned long O2Checkpoint;
  unsigned long O2Actionpoint;
  unsigned long NStartpoint;
  float O2Reading;
  boolean NOn;
  boolean NStepping;
  boolean NStarted;
  // Alarms
  boolean AlarmTempSensorMalfunction;
  boolean AlarmHeatDoorOverTemp;
  boolean AlarmHeatDoorUnderTemp;
  boolean AlarmHeatChamberOverTemp;
  boolean AlarmHeatChamberUnderTemp;
  boolean AlarmCO2SensorMalfunction;
  boolean AlarmCO2OverSaturation;
  boolean AlarmCO2UnderSaturation;
  boolean AlarmO2SensorMalfunction;
  boolean AlarmO2OverSaturation;
  boolean AlarmO2UnderSaturation;
  boolean PreviousAlarmHeat;
  boolean PreviousAlarmCO2;
  boolean PreviousAlarmO2;
};

// Limited globals
SettingsStruct settingsHolder;
StatusStruct statusHolder;

////////* DS18B20 TEMP SENSOR *////////
OneWire oneWire(PINASSIGN_ONEWIRE_BUS); // Setup a oneWire instance to communicate with ANY OneWire devices
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature.

////////* COZIR CO2 SENSOR *////////
SoftwareSerial nss(PINASSIGN_CO2SENSOR_RX, PINASSIGN_CO2SENSOR_TX); // Rx,Tx

////////* O2 SENSOR *////////
SoftwareSerial oss(PINASSIGN_O2SENSOR_RX, PINASSIGN_O2SENSOR_TX); // Rx,Tx

/////////* LCD DISPLAY AND MENU *///////// 
LiquidTWI2 lcd(0);

void setup()
{
  // Startup the Fan, Heaters and Solenoid
  pinMode(PINASSIGN_FAN, OUTPUT);          // Set pin for output to control TIP120 Base pin
  //digitalWrite(PINASSIGN_FAN, HIGH);       // Turn on the Fan
  digitalWrite(PINASSIGN_FAN, LOW);        // Turn off the Fan
  pinMode(PINASSIGN_HEATCHAMBER, OUTPUT);  // Sets pin for controlling heater relay
  digitalWrite(PINASSIGN_HEATCHAMBER, LOW);// Set LOW (heater off)
  pinMode(PINASSIGN_HEATDOOR,OUTPUT);      //Sets pin for controlling Door relay
  digitalWrite(PINASSIGN_HEATDOOR, LOW);   // Set LOW (Door heater off)
  pinMode(PINASSIGN_CO2, OUTPUT);          // Sets pin for controlling CO2 solenoid relay
  digitalWrite(PINASSIGN_CO2, LOW);        // Set LOW (solenoid closed off)
  pinMode(PINASSIGN_NRELAY, OUTPUT);       // Sets pin for controlling Nitrogen solenoid relay
  digitalWrite(PINASSIGN_NRELAY, LOW);     // Set LOW (solenoid closed off)
  
  // Start serial port
  Serial.begin(9600);
  
  // Starting the temperature sensors
  sensors.begin();

  // Starting the CO2 sensor  
  nss.begin(9600);
  
  // Starting the O2 sensor  
  oss.begin(9600); 
  
  // Setting up the LCD
  lcd.setMCPType(LTI_TYPE_MCP23017);
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.print(F("Incuvers Model 1"));
  lcd.setCursor(0,1);
  lcd.print(F("      V1.8     "));  
  Wire.begin(); // wake up I2C bus
  Wire.beginTransmission(0x20);
  Wire.write(0x00); // IODIRA register
  Wire.write(0x01); // set all of port A to inputs
  Wire.endTransmission(); 
  delay(1000);
  lcd.clear(); 

  if (DEBUG_GENERAL) {
    Serial.println(F("Debug build!"));
    lcd.setCursor(0,0);
    lcd.print(F("  Debug build"));
    displayLoadingBar();
    lcd.clear();   
  }

  // Retrieve settings or load defaults
  int RunMode = 0; // 0 = uninitialized
                 // 1 = customized
                 // 2 = defaults
                 // 3 = badConfig
                 // 4 = Updating
  RunMode = doLoadSettings();
  doCheckSettings();
  
  lcd.setCursor(0,0);
  switch (RunMode) {
    case 0 :
      lcd.print(F(" Uninitialized"));
      break;
    case 1 :
      lcd.print(F(" Saved Settings"));
      break;
    case 2 :
      lcd.print(F("Default Settings"));
      break;
    case 3 :
      lcd.print(F("  Bad Config!"));
      break;
    case 4 :
      lcd.print(F("    Updating"));
      break;
  }
  displayLoadingBar();
  lcd.clear(); 
  
  if (RunMode == 2 || RunMode == 3 || RunMode == 4 || RunMode == 0) {
    // Don't have the info we need, load default settings and go into setup
    doResetToDefaults();
    EnterSetupMode();
  }

  // Set the status to safe values.
  statusHolder.LastLoopStartTime = 0;
  statusHolder.DoorCheckpoint = 0; 
  statusHolder.ChamberCheckpoint = 0; 
  statusHolder.TempDoor = 0.0;
  statusHolder.TempChamber = 0.0;
  statusHolder.DoorHeatOn = false;
  statusHolder.DoorHeatStepping = false;
  statusHolder.ChamberHeatOn = false;
  statusHolder.ChamberHeatStepping = false;
  statusHolder.CO2Checkpoint = 0;
  statusHolder.CO2Startpoint = 0;
  statusHolder.CO2Actionpoint = 0;
  statusHolder.CO2Reading = 0.0;
  statusHolder.CO2On = false;
  statusHolder.CO2Stepping = false;
  statusHolder.CO2Started = false;
  statusHolder.O2Checkpoint = 0;
  statusHolder.NStartpoint = 0;
  statusHolder.O2Actionpoint = 0;
  statusHolder.O2Reading = 0.0;
  statusHolder.NOn = false;
  statusHolder.NStepping = false;
  statusHolder.NStarted = false;
  statusHolder.AlarmTempSensorMalfunction = false;
  statusHolder.AlarmHeatDoorOverTemp = false;
  statusHolder.AlarmHeatDoorUnderTemp = false;
  statusHolder.AlarmHeatChamberOverTemp = false;
  statusHolder.AlarmHeatChamberUnderTemp = false;
  statusHolder.AlarmCO2SensorMalfunction = false;
  statusHolder.AlarmCO2OverSaturation = false;
  statusHolder.AlarmCO2UnderSaturation = false;
  statusHolder.AlarmO2SensorMalfunction = false;
  statusHolder.AlarmO2OverSaturation = false;
  statusHolder.AlarmO2UnderSaturation = false;
  statusHolder.PreviousAlarmHeat = false;
  statusHolder.PreviousAlarmCO2 = false;
  statusHolder.PreviousAlarmO2 = false;
}

void loop()
{
  unsigned long nowTime = millis();

  if (nowTime < statusHolder.LastLoopStartTime) {
    if (DEBUG_GENERAL) {
      Serial.println(F("Detected Millis() reset.  48-day run length? resetting the LastLoopStartTime"));
      // TODO: Make sure that the checkpoints will rollover to 0 when calculating them.
      statusHolder.LastLoopStartTime = 0;
    } 
  }
  
  if (DEBUG_GENERAL) {
    Serial.print(F("New loop run: "));
    Serial.print((nowTime-statusHolder.LastLoopStartTime));
    Serial.println(F("ms delta"));
  }

  statusHolder.LastLoopStartTime = nowTime;

  // Verify if timers are done and shut things as requried.
  if (settingsHolder.heatEnable) {
    if (statusHolder.ChamberHeatOn && statusHolder.ChamberHeatStepping) {
      if (nowTime >= statusHolder.ChamberCheckpoint) {
        digitalWrite(PINASSIGN_HEATCHAMBER, LOW);
        if (DEBUG_GENERAL) {
          Serial.print(F("Chamber heater stepping shutdown: "));
          Serial.print((nowTime-statusHolder.DoorCheckpoint));
          Serial.println(F("ms beyond checkpoint"));
        }
        statusHolder.ChamberHeatOn = false;
        statusHolder.ChamberHeatStepping = false;
      }
    }

    if (statusHolder.DoorHeatOn && statusHolder.DoorHeatStepping) {
      if (nowTime >= statusHolder.DoorCheckpoint) {
        digitalWrite(PINASSIGN_HEATDOOR, LOW);
        if (DEBUG_GENERAL) {
          Serial.print(F("Door heater stepping shutdown: "));
          Serial.print((nowTime-statusHolder.DoorCheckpoint));
          Serial.println(F("ms beyond checkpoint"));
        }
        statusHolder.DoorHeatOn = false;
        statusHolder.DoorHeatStepping = false;
      }
    }
  }

  if (settingsHolder.CO2Enable) {
    if (statusHolder.CO2On) {
      if (nowTime >= statusHolder.CO2Checkpoint) {
        digitalWrite(PINASSIGN_CO2, LOW);
        if (DEBUG_GENERAL) {
          Serial.print(F("CO2 shutdown: "));
          Serial.print((nowTime-statusHolder.CO2Checkpoint));
          Serial.println(F("ms beyond checkpoint"));
        }
        statusHolder.CO2On = false;
        statusHolder.CO2Actionpoint = nowTime;
      }
    }
  }

  if (settingsHolder.O2Enable) {
    if (statusHolder.NOn) {
      if (nowTime >= statusHolder.O2Checkpoint) {
        digitalWrite(PINASSIGN_NRELAY, LOW);
        if (DEBUG_GENERAL) {
          Serial.print(F("O2 shutdown: "));
          Serial.print((nowTime-statusHolder.O2Checkpoint));
          Serial.println(F("ms beyond checkpoint"));
        }
        statusHolder.NOn = false;
        statusHolder.O2Actionpoint = nowTime;
      }
    }
  }

  // Update the status
  if (settingsHolder.heatEnable) {
    GetTemperatureReadings();
  }

   if (settingsHolder.CO2Enable) {
    GetCO2Reading();
   }

   if (settingsHolder.O2Enable) {
    GetO2ReadingDigital();
   }

  // Set up the next heater operation
  if (settingsHolder.heatEnable) {
    // Chamber Temperature
    if (statusHolder.TempChamber < settingsHolder.heatSetPoint) {
      if (statusHolder.TempChamber > (settingsHolder.heatSetPoint * TEMPERATURE_STEP_THRESH)) {
        // stepping heat
        if (statusHolder.ChamberHeatOn) {
          // Already in stepping mode
        } else {
          // Stepping mode enabled!
          digitalWrite(PINASSIGN_HEATCHAMBER, HIGH);
          statusHolder.ChamberHeatOn = true;
          statusHolder.ChamberHeatStepping = true;
          statusHolder.ChamberCheckpoint = nowTime + TEMPERATURE_STEP_LEN;
          if (DEBUG_GENERAL) {
            Serial.println(F("HHHHHHHH:Chamber heater stepping mode."));
          }
        }
      } else {
        // non-stepping heat
        if (statusHolder.ChamberHeatOn) {
          // already engaged, check for delta
          if (statusHolder.ChamberCheckpoint < nowTime) {
            statusHolder.AlarmHeatChamberUnderTemp = true;
          }
        } else {
          digitalWrite(PINASSIGN_HEATCHAMBER, HIGH);
          statusHolder.ChamberHeatOn = true;
          statusHolder.ChamberHeatStepping = false;
          statusHolder.ChamberCheckpoint = nowTime + ALARM_TEMP_ON_PERIOD;
          if (DEBUG_GENERAL) {
            Serial.println(F("HHHHHHHH:Chamber heater starting."));
          }
        }
      }
    } else {
      // Chamber temp. is over setpoint.
      if (statusHolder.ChamberHeatOn && !statusHolder.ChamberHeatStepping) {
        // turn off the heater
        digitalWrite(PINASSIGN_HEATCHAMBER, LOW);
        statusHolder.ChamberHeatOn = false;
        if (DEBUG_GENERAL) {
          Serial.println(F("HHHHHHHH:Chamber heater shutdown."));
        }
      }
      if (statusHolder.TempChamber > (settingsHolder.heatSetPoint * ALARM_THRESH)) {
        // Alarm
        statusHolder.AlarmHeatChamberOverTemp = true;
        if (DEBUG_GENERAL) {
          Serial.println(F("HHHHHHHH:Chamber over-temp alarm thrown."));
        }
      }
    }

    // Door Temperature
    if (statusHolder.TempDoor < settingsHolder.heatSetPoint) {
      if (statusHolder.TempDoor > (settingsHolder.heatSetPoint * TEMPERATURE_STEP_THRESH)) {
        // stepping heat
        if (statusHolder.DoorHeatOn) {
          // Already in stepping mode
        } else {
          // Stepping mode enabled!
          digitalWrite(PINASSIGN_HEATDOOR, HIGH);
          statusHolder.DoorHeatOn = true;
          statusHolder.DoorHeatStepping = true;
          statusHolder.DoorCheckpoint = nowTime + TEMPERATURE_STEP_LEN;
          if (DEBUG_GENERAL) {
            Serial.println(F("DDDDDDDD:Door heater stepping mode."));
          }
        }
      } else {
        // non-stepping heat
        if (statusHolder.DoorHeatOn) {
          // already engaged, check for delta
          if (statusHolder.DoorCheckpoint < nowTime) {
            statusHolder.AlarmHeatDoorUnderTemp = true;
          }
        } else {
          digitalWrite(PINASSIGN_HEATDOOR, HIGH);
          statusHolder.DoorHeatOn = true;
          statusHolder.DoorHeatStepping = false;
          statusHolder.DoorCheckpoint = nowTime + ALARM_TEMP_ON_PERIOD;
          if (DEBUG_GENERAL) {
            Serial.println(F("DDDDDDDD:Door heater starting."));
          }
        }
      }
    } else {
      // Chamber temp. is over setpoint.
      if (statusHolder.DoorHeatOn && !statusHolder.DoorHeatStepping) {
        // turn off the heater
        digitalWrite(PINASSIGN_HEATDOOR, LOW);
        statusHolder.DoorHeatOn = false;
        if (DEBUG_GENERAL) {
          Serial.println(F("DDDDDDDD:Door heater shutdown."));
        }
      }
      if (statusHolder.TempDoor > (settingsHolder.heatSetPoint * ALARM_THRESH)) {
        // Alarm
        statusHolder.AlarmHeatDoorOverTemp = true;
        if (DEBUG_GENERAL) {
          Serial.println(F("DDDDDDDD:Door over-temp alarm thrown."));
        }
      }
    }
  }

  // CO2 maintenance
  if (settingsHolder.CO2Enable) {
    if (statusHolder.CO2Reading < settingsHolder.CO2SetPoint ) {
      if (statusHolder.CO2Reading > (settingsHolder.CO2SetPoint * CO2_STEP_THRESH)) {
        if (nowTime > statusHolder.CO2Actionpoint + CO2_BLEEDTIME_STEPPING) {
          // In stepping mode and not worried about bleed delay.
          digitalWrite(PINASSIGN_CO2, HIGH);
          delay(CO2_DELTA_STEPPING);
          digitalWrite(PINASSIGN_CO2, LOW);
          statusHolder.CO2Actionpoint = nowTime;
          if (DEBUG_GENERAL) {
            Serial.println(F("CCCCCCCC:CO2 stepping mode activated."));
          }
        } // there is no else, we need to wait for the bleedtime to expire.
      } else {
        // below the setpoint and the stepping threshold, 
        if (!statusHolder.CO2On && nowTime > (statusHolder.CO2Actionpoint + CO2_BLEEDTIME_JUMP)) {
          if (statusHolder.CO2Started == false) {
            statusHolder.CO2Started = true;
            statusHolder.CO2Startpoint = nowTime;
          } else {
            if (statusHolder.CO2Startpoint + ALARM_CO2_OPEN_PERIOD < nowTime) {
              statusHolder.AlarmCO2UnderSaturation = true;
              if (DEBUG_GENERAL) {
                Serial.println(F("CCCCCCCC:CO2 under-saturation alarm thrown."));
              }
            }
          }
          statusHolder.CO2On = true;
          statusHolder.CO2Checkpoint = (nowTime + CO2_DELTA_JUMP);
          digitalWrite(PINASSIGN_CO2, HIGH);
          if (DEBUG_GENERAL) {
            Serial.print(F("CCCCCCCC:CO2 opening from "));
            Serial.print(nowTime);
            Serial.print(F(" until "));
            Serial.println(statusHolder.CO2Checkpoint);
          }
        } else {
         if (DEBUG_GENERAL) {
           Serial.println(F("CCCCCCCC:CO2 under-saturated but bleed time remaining."));
         } 
        }
      }
    } else {
      // CO2 level above setpoint.
      digitalWrite(PINASSIGN_CO2, LOW); // just to make sure
      statusHolder.CO2Started = false;
      if (statusHolder.CO2Reading > (settingsHolder.CO2SetPoint * ALARM_THRESH)) {
        // Alarm
        statusHolder.AlarmCO2OverSaturation = true;
        if (DEBUG_GENERAL) {
          Serial.println(F("OOOOOOOO:O2 over-saturation alarm thrown."));
        }
      }
    }
  }  

  // O2/N maintenance
  if (settingsHolder.O2Enable) {
    if (statusHolder.O2Reading > settingsHolder.O2SetPoint ) {
      if (statusHolder.O2Reading < (settingsHolder.O2SetPoint * OO_STEP_THRESH)) {
        if (nowTime > statusHolder.O2Actionpoint + N_BLEEDTIME_STEPPING) {
          // In stepping mode and not worried about bleed delay.
          digitalWrite(PINASSIGN_NRELAY, HIGH);
          delay(N_DELTA_STEPPING);
          digitalWrite(PINASSIGN_NRELAY, LOW);
          statusHolder.O2Actionpoint = nowTime;
          if (DEBUG_GENERAL) {
            Serial.println(F("OOOOOOOO:O2 stepping mode activated."));
          }
        } // there is no else, we need to wait for the bleedtime to expire.
      } else {
        // below the setpoint and the stepping threshold, 
        if (!statusHolder.NOn && nowTime > (statusHolder.O2Actionpoint + N_BLEEDTIME_JUMP)) {
          if (statusHolder.NStarted == false) {
            statusHolder.NStarted = true;
            statusHolder.NStartpoint = nowTime;
          } else {
            if (statusHolder.NStartpoint + ALARM_CO2_OPEN_PERIOD < nowTime) {
              statusHolder.AlarmO2UnderSaturation = true;
              if (DEBUG_GENERAL) {
                Serial.println(F("OOOOOOOO:O2 under-saturation alarm thrown."));
              }
            }
          }
          statusHolder.NOn = true;
          statusHolder.O2Checkpoint = (nowTime + N_DELTA_JUMP);
          digitalWrite(PINASSIGN_NRELAY, HIGH);
          if (DEBUG_GENERAL) {
            Serial.print(F("OOOOOOOO:N opening from "));
            Serial.print(nowTime);
            Serial.print(F(" until "));
            Serial.println(statusHolder.O2Checkpoint);
          }
        } else {
         if (DEBUG_GENERAL) {
           Serial.println(F("OOOOOOOO:O2 over-saturated but bleed time remaining."));
         } 
        }
      }
    } else {
      // O2 level below setpoint.
      digitalWrite(PINASSIGN_NRELAY, LOW); // just to make sure
      statusHolder.NStarted = false;
      if (statusHolder.O2Reading > (settingsHolder.O2SetPoint * (1.0 - ALARM_THRESH))) {
        // Alarm
        statusHolder.AlarmO2OverSaturation = true;
        if (DEBUG_GENERAL) {
          Serial.println(F("OOOOOOOO:O2 over-saturation alarm thrown."));
        }
      }
    }
  }  
  
  AlarmOrchestrator();
  LCDDrawDefaultUI(); 
  
  int userInput = GetButtonState();
  if (userInput == 3) {
    EnterSetupMode();  
  }
   
  if (DEBUG_GENERAL) {
    SerialPrintResults();
  }
  
}

// Defaults
void doResetToDefaults()
{
  if (DEBUG_GENERAL) {
    Serial.println(F("Resetting to default values"));
  }
  settingsHolder.heatEnable = true;
  settingsHolder.heatSetPoint = TEMPERATURE_DEF; 
  for (int i = 0; i < 8; i++) {  
    settingsHolder.sensorAddrDoorTemp[i] = 0; 
    settingsHolder.sensorAddrChamberTemp[i] = 0;
  }
  
  settingsHolder.CO2Enable = true;
  settingsHolder.CO2SetPoint = CO2_DEF; 

  settingsHolder.O2Enable = true;
  settingsHolder.O2SetPoint = OO_DEF; 
  
  if (DEBUG_GENERAL) {
    Serial.println(F("Done."));
  }
}

/* H   H EEEEE  AAA  TTTTT
 * H   H E      A A    T
 * HHHHH EEE   AAAAA   T
 * H   H E     A   A   T
 * H   H EEEEE A   A   T
 */

void ShutdownHeatSystem() {
  digitalWrite(PINASSIGN_HEATDOOR, LOW);     // Set LOW (heater off)
  digitalWrite(PINASSIGN_HEATCHAMBER, LOW);  // Set LOW (heater off)
  statusHolder.DoorHeatOn = false;
  statusHolder.DoorHeatStepping = false;
  statusHolder.ChamberHeatOn = false;
  statusHolder.ChamberHeatStepping = false;
}

void GetTemperatureReadings() {
  if (DEBUG_TEMP) {
    Serial.println(F("Call to GetTemperatureReadings()"));
  }
  boolean updateCompleted = false;
  int i = 0;
  
  while (!updateCompleted) {
    // Request the temperatures
    sensors.requestTemperatures();
    // Record the values
    statusHolder.TempDoor = sensors.getTempC(settingsHolder.sensorAddrDoorTemp);
    statusHolder.TempChamber = sensors.getTempC(settingsHolder.sensorAddrChamberTemp);
    
    if (DEBUG_TEMP) {
      Serial.print(F("Door temp. reading: "));
      Serial.print(statusHolder.TempDoor);
      Serial.print(F("*C  Chamber temp. reading: "));
      Serial.print(statusHolder.TempChamber);
      Serial.println("*C");
    }  
    if (statusHolder.TempDoor < 85.0 && statusHolder.TempChamber < 85.0 && statusHolder.TempDoor > -100 && statusHolder.TempChamber > -100) {
      // we have valid readings
      updateCompleted = true;
    } else {
      i++;
      if (DEBUG_TEMP) {
        Serial.print(i);
        Serial.println(F("Oh Nooo!  The temperature sensors returned an error code/invalid reading, trying again!"));
      } 
      if (i > 10) {
        statusHolder.AlarmTempSensorMalfunction = true;
        updateCompleted = true;
      }
    }
  }
  
}

/*  CCC   000   222
 * C   C 00  0 2   2
 * C     0 0 0   22 
 * C   C 0  00  22
 *  CCC   000  22222
 */

void ShutdownCO2System() {
  digitalWrite(PINASSIGN_CO2, LOW);   // Set LOW (solenoid closed off)
  statusHolder.CO2On = false;
  statusHolder.CO2Stepping = false;
  statusHolder.CO2Started = false;
}

void GetCO2Reading() {
  uint8_t bufferStr[25];
  uint8_t ind = 0;
  String val = "";
  int safety = 0;
  unsigned long emergOut = millis() + ALARM_CO2_READING_PERIOD;
  
  // Flush any data on the Rx
  while(nss.available()) {
    nss.read();
  }
  
  for (int i = 0; i<1; i++) {
    while(bufferStr[ind-1] != 0x0A && millis() < emergOut) {
      if(nss.available()) {
        bufferStr[ind] = nss.read();
      ind++;
      }
    }
    
    // Data in the stream looks like "Z 00400 z 00360"
    // The first number is the filtered value and the number after 
    // the 'z' is the raw value. We want the filtered value
    if (DEBUG_CO2) {
      Serial.print(F("CO2 reading (raw): "));
      for (int i = 0; i<ind; i++) {
        Serial.print(bufferStr[i]);
      }
      Serial.println("");
    }
    
    for(int j=0; j < ind+1; j++) {
      if (bufferStr[j] == 'z'){ //once we hit the 'z' we can stop
        break;
      }
      // ignore 'Z' and white space
      if((bufferStr[j] != 0x5A)&&(bufferStr[j] != 0x20)) {
        val += bufferStr[j]-48; // convert ascii to numerical value. 
      }
    }
    statusHolder.CO2Reading = ((CO2_MULTIPLIER * val.toInt()))/10000;
    ind=0;
    val="";

    if (DEBUG_CO2) {
      Serial.print(" - ");
      Serial.println(statusHolder.CO2Reading);
    }
    // ERROR Handling: In case weird value pops up CO2<0.05% or 
    // CO>100% then take the reading again but only up to 5 times
    if (statusHolder.CO2Reading < 0.01 || statusHolder.CO2Reading > 100) {
      if (DEBUG_CO2) {
        Serial.print(safety);
        Serial.println(F(" Zut alors!  The CO2 sensor returned an invalid reading, trying again!"));
      } 
      safety++;
      if (safety < 5) {
        i=i-1;
      } else {
        statusHolder.AlarmCO2SensorMalfunction = true;
      }
    }
  }
}

/*   000   222
 *  00  0 2   2
 *  0 0 0   22 
 *  0  00  22
 *   000  22222
 */

void ShutdownO2System() {
  digitalWrite(PINASSIGN_NRELAY, LOW);   // Set LOW (solenoid closed off)
  statusHolder.NOn = false;
  statusHolder.NStepping = false;
  statusHolder.NStarted = false;
}

void GetO2ReadingDigital() {
  char inchar;
  String Luminoxstring = "";                 //string to hold incoming data from Luminox-O2 sensor
  boolean Luminox_stringcomplete = false;    //were all data from Luminox-O2 sensor received? Check
  Luminoxstring.reserve(41); 
  unsigned long emergOut = millis() + ALARM_O2_READING_PERIOD;
  int j=0, safety =0;
  int iPrcnt, iErr;
  
  for (int i = 0; i<1; i++) {
  
  while(oss.available() && !Luminox_stringcomplete) {
    inchar = (char)oss.read();      //grab that char
      Luminoxstring += inchar;                  //add the received char to LuminoxString
      j++;
      if (inchar == '\r') {                                   //if the incoming character is a <term>, reset
        if (j > 38 && j < 44) {
          // we have a full string
          Luminoxstring.remove(41);                 //remove any serial string overruns 
          Luminox_stringcomplete = true;
        } else {
          // we didn't get a complete string, start over
          Luminoxstring = "";
          i=0;
        }
                  
      }           
  }
    
    // Data in the stream looks like "O 0211.3 T +29.3 P 1011 % 020.90 e 0000"
    //                                O=ppO2 in mbar   P pressure in mbar
    //                                         T=temp in deg C         e=sensor errors
    //                                                        %=O2 concentration in %
    if (DEBUG_O2) {
      Serial.print(F("O2 reading (raw): "));
      Serial.print(Luminoxstring);
      Serial.println("");
    }

    iPrcnt = Luminoxstring.lastIndexOf('%');
    iErr = Luminoxstring.lastIndexOf('e', iPrcnt);

    String readingValue = Luminoxstring.substring(iPrcnt+1, iErr);
    readingValue.trim();
    float readingTest = readingValue.toFloat();

    if (readingTest < 0.01 || readingTest > 25.0) {
      // error
      if (DEBUG_O2) {
        Serial.print(safety);
        Serial.println(F(" Oh Oh Spaghettios!  The O2 sensor returned an invalid reading, trying again!"));
      } 
      safety++;
      if (safety < 5) {
        i=i-1;
      } else {
        //statusHolder.AlarmO2SensorMalfunction = true;
      }
    } else {
    statusHolder.O2Reading = readingTest;
    
    if (DEBUG_O2) {
      Serial.print("Calculated O2 reading - ");
      Serial.println(statusHolder.O2Reading);
    }
    
  }
  }
}

/* U   U III
 * U   U  I
 * U   U  I
 * U   U  I
 *  UUU  III
 */
void LCDDrawDefaultUI() {
  if (statusHolder.personalityCount == 3) {
    LCDDrawNewUI();
  }
  if (statusHolder.personalityCount < 3) {
    LCDDrawDualLineUI(settingsHolder.heatEnable, settingsHolder.CO2Enable, settingsHolder.O2Enable);
  }
}

void LCDDrawDualLineUI(boolean showTemp, boolean showCO2, boolean showO2) {  
  int rowI = 0;
  if (showTemp) {
    lcd.setCursor(0, rowI);
    lcd.print("Temp: ");
    if (settingsHolder.heatEnable) {
      lcd.print(statusHolder.TempChamber, 1);
      lcd.print("\337C  ");
      if (statusHolder.TempChamber < 10.0) {
        lcd.print(" ");
      }
      lcd.print(GetIndicator(statusHolder.DoorHeatOn, statusHolder.DoorHeatStepping, true));
      lcd.print(GetIndicator(statusHolder.ChamberHeatOn, statusHolder.ChamberHeatStepping, false));
    } else if (statusHolder.AlarmTempSensorMalfunction){
      lcd.print(F("Error   "));
    } else {
      lcd.print(F("Disabled"));
    }
    rowI++;
  }
  if (showCO2) {
    lcd.setCursor(0, rowI);
    lcd.print(" CO2: ");
    if (settingsHolder.CO2Enable) {
      lcd.print(statusHolder.CO2Reading, 1);
      lcd.print("%    ");
      if (statusHolder.CO2Reading < 10.0) {
        lcd.print(" ");
      }
      lcd.print(GetIndicator(statusHolder.CO2Started, statusHolder.CO2Stepping, false)); 
    } else if (statusHolder.AlarmCO2SensorMalfunction){
      lcd.print(F("Error   "));
    } else {
      lcd.print(F("Disabled"));
    }
    rowI++;
  }
  if (showO2) {
    lcd.setCursor(0, rowI);
    lcd.print(" O2: ");
    if (settingsHolder.O2Enable) {
      lcd.print(statusHolder.O2Reading, 1);
      lcd.print("%    ");
      if (statusHolder.O2Reading < 10.0) {
        lcd.print(" ");
      }
      lcd.print(GetIndicator(statusHolder.NStarted, statusHolder.NStepping, false)); 
    } else if (statusHolder.AlarmO2SensorMalfunction){
      lcd.print(F("Error   "));
    } else {
      lcd.print(F("Disabled"));
    }
    rowI++;
  }
}

void LCDDrawNewUI() {  
  /*
   * 0123456789ABCDEF
   * T.*+  CO2+   O2+
   * 35.5  10.5  18.2
   * 
   */
  lcd.setCursor(0, 0);
  lcd.print("T.");
  lcd.print(GetIndicator(statusHolder.DoorHeatOn, statusHolder.DoorHeatStepping, true));
  lcd.print(GetIndicator(statusHolder.ChamberHeatOn, statusHolder.ChamberHeatStepping, false));
  lcd.print("  CO2");
  lcd.print(GetIndicator(statusHolder.CO2Started, statusHolder.CO2Stepping, false)); 
  lcd.print("   O2");
  lcd.print(GetIndicator(statusHolder.NStarted, statusHolder.NStepping, false)); 
  lcd.setCursor(0, 1);
  if (statusHolder.TempChamber > 60.0 || statusHolder.TempChamber < -20.0 || statusHolder.AlarmTempSensorMalfunction) {
    lcd.print(" err  ");
  } else {
    if (statusHolder.TempChamber < 10.0) {
      lcd.print(" ");
    }
    lcd.print(statusHolder.TempChamber, 1);
    lcd.print("  ");
  }

  if (statusHolder.AlarmCO2SensorMalfunction) {
    lcd.print(" err  ");
  } else {
    lcd.print(statusHolder.CO2Reading, 1);
    lcd.print("  ");
    if (statusHolder.CO2Reading < 10.0) {
      lcd.print(" ");
    }
    
  }
  
  if (statusHolder.AlarmO2SensorMalfunction) {
    lcd.print("err");
  } else {
    if (statusHolder.O2Reading < 10.0) {
      lcd.print(" ");
    }
    lcd.print(statusHolder.O2Reading, 1);
    lcd.print("  ");
  }
}


char GetIndicator(boolean enabled, boolean stepping, boolean altSymbol) {
  char r = ' ';

  if (enabled && !stepping) {
    if (altSymbol) {
      r = '#';
    } else {
      r = '*';
    }
  } else if (enabled && stepping) {
    if (altSymbol) {
      r = '=';
    } else {
      r = '+';
    }
  }

  return r;
}

void displayLoadingBar() {
  for (int s = 0; s<16; s++) {
    lcd.setCursor(s,1);
    lcd.print(".");
    delay(MENU_UI_LOAD_DELAY);
  }
}

void AlarmOrchestrator() {
  if (statusHolder.AlarmTempSensorMalfunction == true) {
    // There's not much we can do for this one but turn off/disable the heating system.
    ShutdownHeatSystem();
    settingsHolder.heatEnable = false;
  }

  if (statusHolder.AlarmCO2SensorMalfunction == true) {
    // There's not much we can do for this one but turn off/disable the heating system.
    ShutdownCO2System();
    settingsHolder.CO2Enable = false;
  }

  if (statusHolder.AlarmHeatDoorOverTemp == true || 
      /*statusHolder.AlarmHeatDoorUnderTemp == true || */
      statusHolder.AlarmHeatChamberOverTemp == true /*|| 
      statusHolder.AlarmHeatChamberUnderTemp == true*/) {
    ShutdownHeatSystem();
    ShutdownCO2System(); 
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp. Error");
    lcd.setCursor(0, 1);
    lcd.print(statusHolder.TempChamber, 1);
    lcd.print("\337C | ");
    lcd.print(statusHolder.TempDoor, 1);
    lcd.print("\337C");
    
    while (GetButtonState() != 3){
      Wire.beginTransmission(0x20); // Connect to chip
      Wire.write(0x13);             // Address port B
      Wire.write(0x01);                // Sound On
      Wire.endTransmission();       // Close connection
      delay(500);
      Wire.beginTransmission(0x20); // Connect to chip
      Wire.write(0x13);             // Address port B
      Wire.write(0x00);                // Sound On
      Wire.endTransmission();       // Close connection
      delay(500);
    }   
  }

  if (statusHolder.AlarmCO2OverSaturation == true || 
      statusHolder.AlarmCO2UnderSaturation == true) {
    ShutdownHeatSystem();
    ShutdownCO2System(); 
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CO2 Error");
    lcd.setCursor(0, 1);
    lcd.print("CO2: ");
    lcd.print(statusHolder.CO2Reading, 1);
    lcd.print("% ");
    
    while (GetButtonState() != 3){
      Wire.beginTransmission(0x20); // Connect to chip
      Wire.write(0x13);             // Address port B
      Wire.write(0x01);                // Sound On
      Wire.endTransmission();       // Close connection
      delay(500);
      Wire.beginTransmission(0x20); // Connect to chip
      Wire.write(0x13);             // Address port B
      Wire.write(0x00);                // Sound On
      Wire.endTransmission();       // Close connection
      delay(500);
    }    
  }
  
}

// For debugging only
void SerialPrintResults() {
  Serial.println(F("-----------------------------------------------"));
  Serial.println();
  Serial.print(F("Time in operation "));
  Serial.print(millis());
  Serial.println("ms");
  Serial.print(F("Door Temperature is: "));
  Serial.print(statusHolder.TempDoor);
  Serial.print("*C  ");
  Serial.println(GetIndicator(statusHolder.DoorHeatOn, statusHolder.DoorHeatStepping, true));
  Serial.print(F("Chamber Temperature is: "));
  Serial.print(statusHolder.TempChamber);
  Serial.print("*C  ");
  Serial.println(GetIndicator(statusHolder.ChamberHeatOn, statusHolder.ChamberHeatStepping, false));
  Serial.print(F("CO2 Content is: "));
  Serial.print(statusHolder.CO2Reading, 1); 
  Serial.print("% ");
  Serial.println(GetIndicator(statusHolder.CO2On, statusHolder.CO2Stepping, false));
  Serial.println(F("Alarm Status:"));
  Serial.println(F("       HEAT       |        CO2       "));
  if (statusHolder.AlarmTempSensorMalfunction == true || 
      statusHolder.AlarmHeatDoorOverTemp == true || 
      statusHolder.AlarmHeatDoorUnderTemp == true || 
      statusHolder.AlarmHeatChamberOverTemp == true || 
      statusHolder.AlarmHeatChamberUnderTemp == true) {
    Serial.print(F("       yes        |"));
  } else {
    Serial.print(F("                  |"));
  }
  if (statusHolder.AlarmCO2OverSaturation == true || 
      statusHolder.AlarmCO2UnderSaturation == true) {
     Serial.println(F("       yes        "));
  } else {
    Serial.println(F("                  "));
  }
  Serial.println("");
}

int PollButton() {
    Wire.beginTransmission(0x20); // Connect to chip
    Wire.write(0x12);             // Address port A
    Wire.endTransmission();       // Close connection
    Wire.requestFrom(0x20, 1);    // Request one Byte
    return(Wire.read());          // Return the state 
                                  //  0 = no buttons pushed
                                  //  1 = upper button pushed
                                  //  2 = lower button pushed
                                  //  3 = both buttons pushed
}

int GetButtonState() {
  // instead of reading the button once which can cause misreads during a button change operation, if the first reading is not 0, take two more readings a few moments apart to verify what the user wants to do.
  int f, s, t, r;

  r = 0;
  
  f = PollButton();

  if (f != 0) {
    // let's do some extra polling, to make sure we get the right value.
    delay(200);
    s = PollButton();
    delay(200);
    t = PollButton();
  
    if (f == 3 || s == 3 || t == 3) {
      r = 3;
    } else if (f == 2 || s == 2 || t == 2) {
      r = 2;
    } else if (f == 1 || s == 1 || t == 1) {
      r = 1;
    }  
  }
  
  return r;
}

/* M   M EEEEE N   N U   U
 * MM MM E     NN  N U   U
 * M M M EEE   N N N U   U
 * M   M E     N  NN U   U
 * M   M EEEEE N   N  UUU
 */

void EnterSetupMode() {
  ShutdownO2System();
  ShutdownCO2System();
  ShutdownHeatSystem();
  
  SetupLoop();
}


void doSubMenuAdjust(int screenId) {
  boolean doLoop = true;
  boolean redraw = true;
  int userInput;

  while (doLoop) {
    if (redraw) {
      lcd.clear();
      switch (screenId) {
        case 1 :
          lcd.setCursor(0, 0);
          lcd.print(F("Temperature:"));
          lcd.setCursor(2, 1);
          lcd.print(settingsHolder.heatSetPoint, 1);
          break;
        case 2 :
          lcd.setCursor(0, 0);
          lcd.print(F("CO2:"));
          lcd.setCursor(2, 1);
          lcd.print(settingsHolder.CO2SetPoint, 1);
          break;
        case 3 :
          lcd.setCursor(0, 0);
          lcd.print(F("O2:"));
          lcd.setCursor(2, 1);
          lcd.print(settingsHolder.O2SetPoint, 1);
          break;
      }
      lcd.setCursor(15, 0);
      lcd.print("+");
      lcd.setCursor(15, 1);
      lcd.print("-");
      delay(500);
    }
   
    userInput = GetButtonState();
    
    switch (userInput) {
      case 0:
        redraw=false;
        break;
      case 1:
        // Up
        switch (screenId) {
          case 1:
            settingsHolder.heatSetPoint = settingsHolder.heatSetPoint + 0.5;
            break;
          case 2:
            settingsHolder.CO2SetPoint = settingsHolder.CO2SetPoint + 0.1;
            break;
          case 3:
            settingsHolder.O2SetPoint = settingsHolder.O2SetPoint + 0.1;
            break;
        }
        redraw = true;
        break;
      case 2:
        // Down
        switch (screenId) {
          case 1:
            settingsHolder.heatSetPoint = settingsHolder.heatSetPoint - 0.5;
            break;
          case 2:
            settingsHolder.CO2SetPoint = settingsHolder.CO2SetPoint - 0.1;
            break;
          case 3:
            settingsHolder.O2SetPoint = settingsHolder.O2SetPoint - 0.1;
            break;
        }
        redraw = true;
        break;
      case 3:
        // exit
        doLoop=false;
        break;
    }
    
    // Limit values 
    switch (screenId) {
      case 1:
        if (settingsHolder.heatSetPoint < TEMPERATURE_MIN) { settingsHolder.heatSetPoint = TEMPERATURE_MIN; }
        if (settingsHolder.heatSetPoint > TEMPERATURE_MAX) { settingsHolder.heatSetPoint = TEMPERATURE_MAX; }
        break;
      case 2:
        if (settingsHolder.CO2SetPoint < CO2_MIN) { settingsHolder.CO2SetPoint = CO2_MIN; }
        if (settingsHolder.CO2SetPoint > CO2_MAX) { settingsHolder.CO2SetPoint = CO2_MAX; }
        break;
      case 3:
        if (settingsHolder.O2SetPoint < OO_MIN) { settingsHolder.O2SetPoint = OO_MIN; }
        if (settingsHolder.O2SetPoint > OO_MAX) { settingsHolder.O2SetPoint = OO_MAX; }
        break;
      
    }
  }
}

boolean IsSameOWAddress(byte addrA[8], byte addrB[8]) {
  boolean isTheSame = true;
  for (int i=0; i<8; i++) {
    if (addrA[i] != addrB[i]) {
      if (DEBUG_GENERAL) {
        Serial.println("");
        Serial.print(addrA[i], HEX);
        Serial.print(" != ");
        Serial.print(addrB[i], HEX);
      }
      isTheSame = false;
      i = 42;
    }
  }
  return isTheSame;
}  
  

void doTempSensorAssign() {
  int i, j, k;
  byte addrA[2][8];
  byte addr[8];
  boolean doLoop = true;
  boolean redraw = true;
  int userInput;
  int serialZeros;
  
  j=0;
  while(oneWire.search(addr)) {
    for( i = 0; i < 8; i++) {
      addrA[j][i] = addr[i]; 
    }
    j++;
  }
  
  if (DEBUG_GENERAL) {
    Serial.print("Found ");
    Serial.print(j);
    Serial.println(F(" OneWire devices"));
  }
  
  if (j == 0 ) {
    // no temp sensors
    settingsHolder.heatEnable = false;
  } else if (j >= 1) {
    serialZeros = 0;
    for (i = 0; i < 8; i++) {
      if (addrA[0][i] == 0) {
        serialZeros++;
      }
    }
    if (serialZeros < 6) {
      if (DEBUG_GENERAL) {
        Serial.print(F("Setting Door Sensor Serial: "));
      }
      for (k = 0; k < 8; k++) {
        settingsHolder.sensorAddrDoorTemp[k] = addrA[0][k];
        if (DEBUG_GENERAL) {
          Serial.print(settingsHolder.sensorAddrDoorTemp[k]);
          Serial.print(":");
        }
      }
      Serial.println(F("."));
    }
  } 
  
  if (j >= 2) {
    serialZeros = 0;
    for (i = 0; i < 8; i++) {
      if (addrA[1][i] == 0) {
        serialZeros++;
      }
    }
    if (serialZeros < 6) {
      if (DEBUG_GENERAL) {
        Serial.print(F("Setting Chamber Sensor Serial: "));
      }
      for (k = 0; k < 8; k++) {
        settingsHolder.sensorAddrChamberTemp[k] = addrA[1][k];
        if (DEBUG_GENERAL) {
          Serial.print(settingsHolder.sensorAddrChamberTemp[k]);
          Serial.print(":");
        }
      }
      Serial.println(F("."));
    }
  }
   
  while (doLoop) {
    if (redraw) {
      GetTemperatureReadings();
      lcd.setCursor(0, 0);
      lcd.print("Door: ");
      lcd.print(statusHolder.TempDoor, 1);
      lcd.print("    ");
      lcd.setCursor(0, 1);
      lcd.print("Chmbr: ");
      lcd.print(statusHolder.TempChamber, 1);
      lcd.print("    ");
      delay(250);
    }
   
    userInput = GetButtonState();
    
    switch (userInput) {
      case 0:
        delay(100);
        redraw=true;
        break;
      case 1:
        // Swap
        for (i = 0; i < 8; i++) {
          addr[i] = settingsHolder.sensorAddrDoorTemp[i];
          settingsHolder.sensorAddrDoorTemp[i] = settingsHolder.sensorAddrChamberTemp[i];
          settingsHolder.sensorAddrChamberTemp[i] = addr[i];
        }
        redraw = true;
        break;
      case 2:
        // Exit
        doLoop=false;
        break;
      case 3:
        // exit
        doLoop=false;
        break;
    }
  }
}

void doFeatureSetHeat()
{
  boolean doLoop = true;
  boolean redraw = true;
  int userInput;
  
  while (doLoop) {
    if (redraw) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Heat: "));
      if (settingsHolder.heatEnable) {
        lcd.print(F("Enabled "));
      } else {
        lcd.print(F("Disabled"));
      }
      lcd.setCursor(0, 1);
      lcd.print(F("                "));
      delay(500);
    }
   
    userInput = GetButtonState();
    
    switch (userInput) {
      case 0:
        delay(100);
        redraw=false;
        break;
      case 1:
        // Toggle Heat
        if (settingsHolder.heatEnable) {
          settingsHolder.heatEnable = false;
        } else {
          settingsHolder.heatEnable = true;
        }
        redraw = true;
        break;
      case 2:
        // Do nothing
        break;
      case 3:
        // exit
        doLoop=false;
        break;
    }
  }
}

void doFeatureSetGas()
{
  boolean doLoop = true;
  boolean redraw = true;
  int userInput;
  
  while (doLoop) {
    if (redraw) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CO2: ");
      if (settingsHolder.CO2Enable) {
        lcd.print("Enabled ");
      } else {
        lcd.print("Disabled");
      }
      lcd.setCursor(0, 1);
      lcd.print("O2: ");
      if (settingsHolder.O2Enable) {
        lcd.print("Enabled ");
      } else {
        lcd.print("Disabled");
      }
      delay(500);
    }
   
    userInput = GetButtonState();
    
    switch (userInput) {
      case 0:
        delay(100);
        redraw=false;
        break;
      case 1:
        // Toggle CO2
        if (settingsHolder.CO2Enable) {
          settingsHolder.CO2Enable = false;
        } else {
          settingsHolder.CO2Enable = true;
        }
        redraw = true;
        break;
      case 2:
        // Toggle O2
        if (settingsHolder.O2Enable) {
          settingsHolder.O2Enable = false;
        } else {
          settingsHolder.O2Enable = true;
        }
        redraw = true;
        break;
      case 3:
        // exit
        doLoop=false;
        break;
    }
  }
}


void SetupLoop() {
  boolean doLoop = true;
  boolean redraw = true;
  int screen = 0;
  int userInput;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Incuvers Setup"));
  displayLoadingBar();
  delay(500);
  while (doLoop) {
    if (screen < 1 || screen > 9) {
      // go to the first screen
      screen = 1;
    }
    if (redraw) {
      switch (screen) {
        case 1:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Set temp.");
          break;
        case 2:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Set CO2");
          break;
        case 3:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Set O2");
          break;
        case 4:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Settings");
          break;
        case 5:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Heating");
          break;
        case 6:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Gas Features");
          break;
        case 7:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Sensors");
          break;  
        case 8:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Defaults");
          break;
        case 9:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("");
          break;
      }
      switch (screen) {
        case 1:
        case 2:
        case 3:
        case 5:
        case 6:
        case 7:
          lcd.setCursor(13, 0);
          lcd.print("Set");
          lcd.setCursor(12, 1);
          lcd.print("Next");
          break;
       case 4:
          lcd.setCursor(12, 0);
          lcd.print("Save");
          lcd.setCursor(8, 1);
          lcd.print("Advanced");
          break;   
       case 8:
          lcd.setCursor(11, 0);
          lcd.print("Reset");
          lcd.setCursor(12, 1);
          lcd.print("Next");
          break;
       case 9:
          lcd.setCursor(12, 0);
          lcd.print("Save");
          lcd.setCursor(2, 1);
          lcd.print("Basic Settings");
          break;   
      }
      delay(250);
    }      
    userInput = GetButtonState();
    switch (userInput) {
      case 0:
        delay(100);
        redraw=false;
        break;
      case 1:
        lcd.clear();
        delay(250);
        // Select
        switch (screen) {
          case 0:
            // main screen, nothing to set here
            break;
          case 1:
            // set Temperature screen
            doSubMenuAdjust(1);
            delay(250);
            break;
          case 2:
            // set CO2 screen
            doSubMenuAdjust(2);
            delay(250);
            break;
          case 3:
            // set O2 screen
            doSubMenuAdjust(3);
            delay(250);
            break;
          case 5:
            // enable/disable Heat features
            doFeatureSetHeat();
            delay(250);
            break;
          case 6:
            // enable/disable Gas features
            doFeatureSetGas();
            delay(250);
            break;
          case 7:
            // set sensors
            doTempSensorAssign();
            delay(250);
            break;
          case 4:
          case 9:
            // save settings
            doSaveSettings();
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Settings saved."));
            lcd.setCursor(0, 1);
            lcd.print(F("Exiting setup..."));  
            doCheckSettings();
            delay(1000);
            doLoop=false;
            break;
          case 8:
            // reset to defaults
            doResetToDefaults();
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Reset to default"));
            displayLoadingBar();
            break;
        }
        redraw = true;
        break;
      case 2:
        // Next
        screen++;
        lcd.clear();
        delay(250);
        redraw = true;
        break;
      case 3:
        // exit
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Exiting setup..."));
        displayLoadingBar();
        doCheckSettings();
        doLoop=false;
        break;
    } 
  }
}

// Settings Handler
void doSaveSettings() {
  if (DEBUG_GENERAL) {
    Serial.println();
    Serial.print(F("Saving settings with ID: "));
    Serial.print(SETTINGS_IDENT);
    Serial.print("...");
  }
  
  // Check
  settingsHolder.ident = SETTINGS_IDENT;
  
  if (DEBUG_GENERAL) {
    Serial.println();
  }

  for (unsigned int i=0; i < sizeof(settingsHolder); i++)
  {
    EEPROM.write(SETTINGS_ADDRS + i, *((char*)&settingsHolder + i));
    if (DEBUG_GENERAL) {
      Serial.print(".");
    }
  }
  
  if (DEBUG_GENERAL) {
    if (DEBUG_GENERAL) {
      Serial.print(F("  Done!"));
      Serial.println();
    }
  }
}

int doLoadSettings() {
  int runMode = 0;
  if (DEBUG_GENERAL) {
    Serial.println();
    Serial.print(F("Reading settings with ID: "));
    Serial.print(SETTINGS_IDENT);
    Serial.print(F(" at offset "));
    Serial.print(SETTINGS_ADDRS);
    Serial.println();
  }
  
  if (doCheckSettingsIdentification(SETTINGS_ADDRS)) {
    for (unsigned int i = 0; i < sizeof(settingsHolder); i++) {
      *((char*)&settingsHolder + i) = EEPROM.read(SETTINGS_ADDRS + i);    
    }
    // Temp Settings
    if (DEBUG_GENERAL) {
      Serial.print(F("Heating Enabled: "));
      Serial.print("");
      Serial.println();
      Serial.print(F("Temperature setpoint: "));
      Serial.print(settingsHolder.heatSetPoint);
      Serial.println();
      Serial.print(F("Door Sensor: "));
      for(int i = 0; i < 8; i++) {
        Serial.print("0x");
        if (settingsHolder.sensorAddrDoorTemp[i] < 16) {
          Serial.print('0');
        }
        Serial.print(settingsHolder.sensorAddrDoorTemp[i], HEX);
        if (i < 7) {
          Serial.print(", ");
        }
      }
      Serial.println();
      Serial.print(F("Chamber Sensor: "));
      for(int i = 0; i < 8; i++) {
        Serial.print("0x");
        if (settingsHolder.sensorAddrChamberTemp[i] < 16) {
          Serial.print('0');
        }
        Serial.print(settingsHolder.sensorAddrChamberTemp[i], HEX);
        if (i < 7) {
          Serial.print(", ");
        }
      }
      Serial.println();
    }
      
    // post-reload work
    runMode = 1;    
    if (DEBUG_GENERAL) {
      Serial.print(F("Settings retrieved."));
      Serial.println();
    }
  } else if (doCheckSettingsOldIdentification(SETTINGS_ADDRS)) {
    doImportOldSettings();
    runMode = 4;
  } else {
    runMode = 2;
    if (DEBUG_GENERAL) {
      Serial.print(F("Settings not found."));
      Serial.println();
    }
  }
  return runMode;
}

void doImportOldSettings() {
  if (DEBUG_GENERAL) {
    Serial.print(F("Importing old settings "));
  }
  SettingsStruct_14 settingsHolder_14;
  for (unsigned int i = 0; i < sizeof(settingsHolder_14); i++) {
    *((char*)&settingsHolder_14 + i) = EEPROM.read(SETTINGS_ADDRS + i);    
  }
  doResetToDefaults();
  settingsHolder.heatEnable = settingsHolder_14.heatEnable;
  settingsHolder.heatSetPoint = settingsHolder_14.heatSetPoint;
  for(int i = 0; i < 8; i++) {
    settingsHolder.sensorAddrDoorTemp[i] = settingsHolder_14.sensorAddrDoorTemp[i];
    settingsHolder.sensorAddrChamberTemp[i] = settingsHolder_14.sensorAddrChamberTemp[i];
  }
  settingsHolder.CO2Enable = settingsHolder_14.CO2Enable;
  settingsHolder.CO2SetPoint = settingsHolder_14.CO2SetPoint;
  
  if (DEBUG_GENERAL) {
    Serial.print(F("Heating Enabled: "));
    Serial.print("");
    Serial.println();
    Serial.print(F("Temperature setpoint: "));
    Serial.print(settingsHolder.heatSetPoint);
    Serial.println();
    Serial.print(F("Door Sensor: "));
    for(int i = 0; i < 8; i++) {
      Serial.print("0x");
      if (settingsHolder.sensorAddrDoorTemp[i] < 16) {
        Serial.print('0');
      }
      Serial.print(settingsHolder.sensorAddrDoorTemp[i], HEX);
      if (i < 7) {
        Serial.print(", ");
      }
    }
    Serial.println();
    Serial.print(F("Chamber Sensor: "));
    for(int i = 0; i < 8; i++) {
      Serial.print("0x");
      if (settingsHolder.sensorAddrChamberTemp[i] < 16) {
        Serial.print('0');
      }
      Serial.print(settingsHolder.sensorAddrChamberTemp[i], HEX);
      if (i < 7) {
        Serial.print(", ");
      }
    }
    Serial.println();
  }
  
}

bool doCheckSettingsIdentification(int startAddress) {
  if (EEPROM.read(startAddress) == SETTINGS_IDENT) {
    return true;
  } else {
    return false;
  }
}

bool doCheckSettingsOldIdentification(int startAddress) {
  if (EEPROM.read(startAddress) == SETTINGS_IDENT_14) {
    return true;
  } else {
    return false;
  }
}

void doCheckSettings() {
  /*
  int serialZeros = 0;
  for (int i = 0; i < 8; i++) {
    if (settingsHolder.sensorAddrDoorTemp[i] == 0) {
      serialZeros++;
    }
  }
  if (serialZeros > 6) {
    settingsHolder.heatEnable = false;
    if (DEBUG_GENERAL) {
      Serial.print(F("Disabling heat due to lack of serial number."));
      Serial.println();
    }
  }
  serialZeros = 0;
  for (int i = 0; i < 8; i++) {
    if (settingsHolder.sensorAddrChamberTemp[i] == 0) {
      serialZeros++;
    }
  }
  if (serialZeros > 6) {
    settingsHolder.heatEnable = false;
    if (DEBUG_GENERAL) {
      Serial.print(F("Disabling heat due to lack of serial number."));
      Serial.println();
    }
  }*/

  statusHolder.personalityCount = 0;
  if (settingsHolder.heatEnable) {
    statusHolder.personalityCount++;
  }
  if (settingsHolder.CO2Enable) {
    statusHolder.personalityCount++;
  }
  if (settingsHolder.O2Enable) {
    statusHolder.personalityCount++;
  }

  if (DEBUG_GENERAL) {
    Serial.print(F("Counted "));
    Serial.print(statusHolder.personalityCount);
    Serial.print(F(" personalities enabled."));
    Serial.println();
  }
}

