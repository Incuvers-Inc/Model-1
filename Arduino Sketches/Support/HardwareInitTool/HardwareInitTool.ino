// Hardware tag to ensure incorrect data is not read.
// M1b = Model 1, release I (for individual sensors.)
#define HARDWARE_IDENT "M1I"
#define HARDWARE_ADDRS 4 

// Structure
struct HardwareStruct {
  byte ident[3];
  // Identification
  byte hVer[3];
  int serial;
  // Temp settings
  byte countOfTempSensors;
  byte sensorAddrOne[8];
  byte sensorAddrTwo[8];
  // CO2 settings
  bool hasCO2Sensor;
  byte CO2RxPin;
  byte CO2TxPin;
  // O2 settings
  bool hasO2Sensor;
  byte O2RxPin;
  byte O2TxPin;
  // Gas Relay
  bool CO2GasRelay;
  byte CO2RelayPin;
  bool O2GasRelay;
  byte O2RelayPin;
  // PiLink
  bool piSupport;
  byte piRxPin;
  byte piTxPin;
  // Lighting
  bool lightingSupport;
  byte lightPin;
};

#include "OneWire.h"
#include "DallasTemperature.h"
#include "Wire.h"
#include "LiquidTWI2.h"
#include <EEPROM.h>  

#include "serialSensor.h"

HardwareStruct hardwareDefinition;
LiquidTWI2 lcd(0);


void PerformSaveHardwareDefinition() {
  Serial.println(F("Call to PerformSaveHardwareDefinition()"));

  for (int i=0; i < 3; i++ ) {
    hardwareDefinition.ident[i] = HARDWARE_IDENT[i];
  }
  
  Serial.print(F("  ID: "));
  Serial.print(HARDWARE_IDENT);
  Serial.print(", ");
  Serial.print(sizeof(hardwareDefinition));
  Serial.print(" bytes");

  for (unsigned int i=0; i < sizeof(hardwareDefinition); i++) {
    EEPROM.write(HARDWARE_ADDRS + i, *((char*)&hardwareDefinition + i));
    Serial.print(".");
  }
  
  Serial.println("  Done.");
  Serial.println(F("End of PerformSaveHardwareDefintion()"));
}

void PerformHardwareInventory() {
  if (hardwareDefinition.countOfTempSensors > 0) {
  
  }

  if (hardwareDefinition.hasCO2Sensor) {
    IncuversSerialSensor* co2Sense = new IncuversSerialSensor();
    co2Sense->Initialize(hardwareDefinition.CO2RxPin, hardwareDefinition.CO2TxPin, "K 2");
    Serial.print("COZIR sensor details: ");
    Serial.println(co2Sense->GetCOZIRSensorDetails());
  }
  
  if (hardwareDefinition.hasO2Sensor) {
    IncuversSerialSensor* o2Sense = new IncuversSerialSensor();
    o2Sense->Initialize(hardwareDefinition.O2RxPin, hardwareDefinition.O2TxPin, "M 1");
    Serial.print("Luminox sensor details: ");
    Serial.println(o2Sense->GetLuminoxSensorDetails());
    Serial.print("Luminox all sense: ");
    Serial.println(o2Sense->GetLuminoxSense());
  }
  
  if (hardwareDefinition.CO2GasRelay) {
  
  }
  
  if (hardwareDefinition.O2GasRelay) {
  
  }
  
  if (hardwareDefinition.piSupport) {
  
  }
  
  if (hardwareDefinition.lightingSupport) {
  
  }
  
}

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

 /* ATMEGA 2560 Pins in use 1.0.1:
 *    D2  - Opt0 Relay
 *    D3  - Opt1 Relay
 *    D4  - DS18B20 temperature sensors
 *    D5  - Opt2 Relay
 *    D6  - GAS1/CO2 Solenoid relay
 *    D7  - GAS2/O2 Solenoid relay
 *    D8  - Door Heater relay
 *    D9  - Chamber Heater relay
 *    D10 - Fan relay
 *    D14 - O2 Sensor Tx
 *    D15 - O2 Sensor Rx
 *    D16 - CO2 Sensor Tx
 *    D17 - CO2 Sesnor Rx
 *    D18 - PiLink Tx
 *    D19 - PiLink Rx
 *    D20 - SDA (I2C) - Comms with MCP23017 I/O port expander  
 *    D21 - SCL (I2C) - Comms with MCP23017 I/O port expander
 */

 /* ATMEGA 2560 Pins in use 1.0.2:
 *    D2  - Opt0 Relay
 *    D3  - Opt1 Relay
 *    D4  - DS18B20 temperature sensor C
 *    D5  - Opt2 Relay
 *    D6  - GAS1/CO2 Solenoid relay
 *    D7  - GAS2/O2 Solenoid relay
 *    D8  - Door Heater relay
 *    D9  - Chamber Heater relay
 *    D10 - Fan relay
 *    D14 - O2 Sensor Tx
 *    D15 - O2 Sensor Rx
 *    D16 - CO2 Sensor Tx
 *    D17 - CO2 Sesnor Rx
 *    D18 - PiLink Tx
 *    D19 - PiLink Rx
 *    D20 - SDA (I2C) - Comms with MCP23017 I/O port expander  
 *    D21 - SCL (I2C) - Comms with MCP23017 I/O port expander
 *    D22 - DS18B20 temperature sensor A
 *    D49 - DS18B20 temperature sensor D
 */
void setup() {
  Serial.begin(9600);
  lcd.setMCPType(LTI_TYPE_MCP23017);
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.print(F("Incuvers Hwrdwr"));
  lcd.setCursor(0,1);
  lcd.print(F("     Tool"));  
  Wire.begin(); // wake up I2C bus
  Wire.beginTransmission(0x20);
  Wire.write(0x00); // IODIRA register
  Wire.write(0xFF); // set all of port A to inputs
  Wire.endTransmission(); 
  delay(1000);
  lcd.clear(); 
  
  // Hardware version information.  
  //      0.9.3 is the original PCB revision for Model 1
  //      1.0.0 is the first Incuvers-sourced PCB
  //      1.0.1 is the first general-use Incuvers PCB
  //      1.0.2 is the board created in 2019. 
//  hardwareDefinition.hVer[0]=%%HVER_A%%;
//  hardwareDefinition.hVer[1]=%%HVER_B%%;
//  hardwareDefinition.hVer[2]=%%HVER_C%%;
  // Hardware serial number
//  hardwareDefinition.serial=%%SERIAL%%;             // Serial number as printed inside the top cover
  // Temperature sensors
//  hardwareDefinition.countOfTempSensors=%%TEMPCOUNT%%;    // Count of temperature sensors installed
//  hardwareDefinition.sensorAddrOne[0] = %%TEMPADDR_A%%;
//  hardwareDefinition.sensorAddrOne[1] = %%TEMPADDR_B%%;
//  hardwareDefinition.sensorAddrOne[2] = %%TEMPADDR_C%%;
//  hardwareDefinition.sensorAddrOne[3] = %%TEMPADDR_D%%;
//  hardwareDefinition.sensorAddrOne[4] = %%TEMPADDR_E%%;
//  hardwareDefinition.sensorAddrOne[5] = %%TEMPADDR_F%%;
//  hardwareDefinition.sensorAddrOne[6] = %%TEMPADDR_G%%;
//  hardwareDefinition.sensorAddrOne[7] = %%TEMPADDR_H%%;
//  for (int i = 0; i <8; i++) {
//    hardwareDefinition.sensorAddrTwo[i] = 0;
//  }
  // CO2 sensor
//  hardwareDefinition.hasCO2Sensor = %%CO2_S_E%%;    // Is there a CO2 sensor present?
//  hardwareDefinition.CO2RxPin     = %%CO2_S_R%%;    // Default: 17
//  hardwareDefinition.CO2TxPin     = %%CO2_S_T%%;    // Default: 16
  // Oxygen sensor
//  hardwareDefinition.hasO2Sensor = %%O2_S_E%%;      // Is there an O2 sensor present?
//  hardwareDefinition.O2RxPin     = %%O2_S_R%%;      // Default: 15
//  hardwareDefinition.O2TxPin     = %%O2_S_T%%;      // Default: 14 
  // Gas Relay
//  hardwareDefinition.CO2GasRelay = %%CO2_V_E%%;     // Is there a gas valve present for CO2 management?
//  hardwareDefinition.CO2RelayPin = %%CO2_V_C%%;     // Default: 6
//  hardwareDefinition.O2GasRelay  = %%O2_V_E%%;      // Is there a gas valve present for O2 management?
//  hardwareDefinition.O2RelayPin  = %%O2_V_C%%;      // Default: 7
  // PiLink
//  hardwareDefinition.piSupport = %%PI_E%%;          // Is there a pi installed?
//  hardwareDefinition.piRxPin   = %%PI_R%%;          // Default: 19
//  hardwareDefinition.piTxPin   = %%PI_T%%;          // Default: 18
  // Lighting
//  hardwareDefinition.lightingSupport = %%LITE_E%%;  // Is there an internal lighting system?
//  hardwareDefinition.lightPin        = %%LITE_C%%;  // Default: 2 

  hardwareDefinition.hVer[0]=1;
  hardwareDefinition.hVer[1]=0;
  hardwareDefinition.hVer[2]=2;
  // Hardware serial number
  hardwareDefinition.serial=99999;             // Serial number as printed inside the top cover
  // Temperature sensors
  hardwareDefinition.countOfTempSensors=1;    // Count of temperature sensors installed
  hardwareDefinition.sensorAddrOne[0] = 1;
  hardwareDefinition.sensorAddrOne[1] = 4;
  hardwareDefinition.sensorAddrOne[2] = 0;
  hardwareDefinition.sensorAddrOne[3] = 0;
  hardwareDefinition.sensorAddrOne[4] = 0;
  hardwareDefinition.sensorAddrOne[5] = 0;
  hardwareDefinition.sensorAddrOne[6] = 0;
  hardwareDefinition.sensorAddrOne[7] = 0;
  for (int i = 0; i <8; i++) {
    hardwareDefinition.sensorAddrTwo[i] = 0;
  }
  // CO2 sensor
  hardwareDefinition.hasCO2Sensor = true;    // Is there a CO2 sensor present?
  hardwareDefinition.CO2RxPin     = 17;    // Default: 17
  hardwareDefinition.CO2TxPin     = 16;    // Default: 16
  // Oxygen sensor
  hardwareDefinition.hasO2Sensor = true;      // Is there an O2 sensor present?
  hardwareDefinition.O2RxPin     = 15;      // Default: 15
  hardwareDefinition.O2TxPin     = 14;      // Default: 14 
  // Gas Relay
  hardwareDefinition.CO2GasRelay = true;     // Is there a gas valve present for CO2 management?
  hardwareDefinition.CO2RelayPin = 6;     // Default: 6
  hardwareDefinition.O2GasRelay  = true;      // Is there a gas valve present for O2 management?
  hardwareDefinition.O2RelayPin  = 7;      // Default: 7
  // PiLink
  hardwareDefinition.piSupport = true;          // Is there a pi installed?
  hardwareDefinition.piRxPin   = 19;          // Default: 19
  hardwareDefinition.piTxPin   = 18;          // Default: 18
  // Lighting
  hardwareDefinition.lightingSupport = false;  // Is there an internal lighting system?
  hardwareDefinition.lightPin        = 2;  // Default: 2 

  delay (1000);
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Saving "));
  lcd.print(sizeof(hardwareDefinition));
  lcd.print(F(" bytes"));
  lcd.setCursor(0,1);
  lcd.print(F("at "));
  lcd.print(HARDWARE_ADDRS);
  lcd.print(F(" w/ ID "));
  lcd.print(HARDWARE_IDENT);
    
  PerformSaveHardwareDefinition();
  delay (1000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Init. complete"));
  lcd.setCursor(0,1);
  lcd.print(hardwareDefinition.serial);
  lcd.print(F(" on "));
  lcd.print(hardwareDefinition.hVer[0]);
  lcd.print(F("."));
  lcd.print(hardwareDefinition.hVer[1]);
  lcd.print(F("."));
  lcd.print(hardwareDefinition.hVer[2]);
  delay(1000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Hrdwre invntr"));
  lcd.setCursor(0,1);
  lcd.print(F("."));
  PerformHardwareInventory();
  lcd.print(F("Done!"));
  delay(1000);
}

void loop() {

  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Done!"));
  delay (1000);

  while (true) {
    // put your main code here, to run repeatedly:
    Serial.println(F("Writing hardware definitions to EEPROM was a success, nothing left to do."));
    lcd.setCursor(0,0);
    lcd.print(F("Please flash w/"));
    lcd.setCursor(0,1);
    lcd.print(F("latest Incuvers"));
    delay(4000);
    lcd.clear();
    delay(1000);
  }
}

