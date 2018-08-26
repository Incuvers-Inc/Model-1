// Hardware settings definitions
#define HARDWARE_IDENT "M1c"
#define HARDWARE_ADDRS 4 

// Settings definitions
#define SETTINGS_IDENT_CURR 111
#define SETTINGS_ADDRS 64

// Defaults
#define TEMPERATURE_DEF 38.0
#define CO2_DEF 5.0
#define OO_DEF 5.0

// Structures
struct HardwareStruct {
  byte ident[3];
  // Identification
  byte hVer[3];
  int serial;
  // Temp settings
  byte countOfTempSensors;
  byte sensorAddrDoorTemp[8];
  byte sensorAddrChamberTemp[8];
  // CO2 settings
  bool hasCO2Sensor;
  byte CO2RxPin;
  byte CO2TxPin;
  // O2 settings
  bool hasO2Sensor;
  byte O2RxPin;
  byte O2TxPin;
  // Gas Relay(s)
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
struct SettingsStruct {
  byte ident;
  // Fan settings
  byte fanMode;       // 0 = off, 1 = on during heat + 30 seconds after, 2 = on during heat + 60 seconds, 3 = on during heat + 50% of time, 4 = on
  // Temp settings
  byte heatMode;      // 0 = off, 1 = on
  float heatSetPoint;
  // CO2 settings
  byte CO2Mode;       // 0 = off, 1 = read, 2 = maintain
  float CO2SetPoint;
  // O2 settings
  byte O2Mode;        // 0 = off, 1 = read, 2 = maintain
  float O2SetPoint;
  // Lighting
  byte lightMode;     // 0 = off, 1 = internal timing, 2 = external timing
  long millisOn;
  long millisOff;
  // Alarm settings
  byte alarmMode;     // 0 = off, 1 = report, 2 = alarm
  
};

class IncuversSettingsHandler {
  private:
    HardwareStruct settingsHardware;
    SettingsStruct settingsHolder;
  
    IncuversHeatingSystem* incHeat;
    IncuversLightingSystem* incLight;
    IncuversCO2System* incCO2;
    IncuversO2System* incO2;
    
    int personalityCount;
    
    int VerifyEEPROMHeader(int startAddress, boolean isHardware) {
      #ifdef DEBUG_EEPROM
        Serial.println(F("VerifyHead"));
      #endif
      
      byte eepromContent[4];
      boolean success = true;
      int ret;
      
      int j;
      if (isHardware) { 
        j = 3;
      } else {
        j = 1;
      }
    
      for (int i = 0; i < j; i++) {
        eepromContent[i] = EEPROM.read(startAddress + i);
      }
    
      if (isHardware) {
        for (int i = 0; i < j; i++) {
          if (eepromContent[i] != HARDWARE_IDENT[i]) {
            success = false;
          }
        }
        if (success) {
          ret = 1;
        } else {
          ret = -1;
        }
      } else {
        if (eepromContent[0] == SETTINGS_IDENT_CURR) {
          ret = (int)eepromContent[0]; 
        } else {
          ret = -1;
        }
      }
      
      #ifdef DEBUG_EEPROM
        Serial.print(F("  /VerifyHead; returning: "));
        Serial.println(ret);
      #endif
      
      return ret;
    }

    boolean ReadHardwareSettings() {
      #ifdef DEBUG_EEPROM
        Serial.println(F("HardwareSet"));
      #endif
      int headerCheck;
      
      headerCheck = VerifyEEPROMHeader(HARDWARE_ADDRS, true);
    
      if (headerCheck == -1) {
        // uninitialized hardware
        #ifdef DEBUG_EEPROM
          Serial.println(F("\tNo hardware settings found, can't run."));
        #endif
        return false;
      } else {
        #ifdef DEBUG_EEPROM
          Serial.print(F("  The hardwareData ident matched, loading "));
          Serial.print(sizeof(settingsHardware));
          Serial.println(F(" bytes of hardware definitions."));
        #endif
        
        for (int i = 0; i < sizeof(settingsHardware); i++) {
          *((char*)&settingsHardware + i) = EEPROM.read(HARDWARE_ADDRS + i);    
        }
        #ifdef DEBUG_EEPROM
        Serial.print(settingsHardware.ident[0]);
        Serial.print(settingsHardware.ident[1]);
        Serial.println(settingsHardware.ident[2]);
        // Identification
        Serial.print(settingsHardware.hVer[0]);
        Serial.print(settingsHardware.hVer[1]);
        Serial.println(settingsHardware.hVer[2]);
        Serial.println(settingsHardware.serial);
        // Temp settings
        Serial.println(settingsHardware.countOfTempSensors);
        Serial.print(settingsHardware.sensorAddrDoorTemp[0]);
        Serial.print(settingsHardware.sensorAddrDoorTemp[1]);
        Serial.print(settingsHardware.sensorAddrDoorTemp[2]);
        Serial.print(settingsHardware.sensorAddrDoorTemp[3]);
        Serial.print(settingsHardware.sensorAddrDoorTemp[4]);
        Serial.print(settingsHardware.sensorAddrDoorTemp[5]);
        Serial.print(settingsHardware.sensorAddrDoorTemp[6]);
        Serial.println(settingsHardware.sensorAddrDoorTemp[7]);
        Serial.print(settingsHardware.sensorAddrChamberTemp[0]);
        Serial.print(settingsHardware.sensorAddrChamberTemp[1]);
        Serial.print(settingsHardware.sensorAddrChamberTemp[2]);
        Serial.print(settingsHardware.sensorAddrChamberTemp[3]);
        Serial.print(settingsHardware.sensorAddrChamberTemp[4]);
        Serial.print(settingsHardware.sensorAddrChamberTemp[5]);
        Serial.print(settingsHardware.sensorAddrChamberTemp[6]);
        Serial.println(settingsHardware.sensorAddrChamberTemp[7]);
        // CO2 settings
        Serial.println(settingsHardware.hasCO2Sensor);
        Serial.println(settingsHardware.CO2RxPin);
        Serial.println(settingsHardware.CO2TxPin);
        // O2 settings
        Serial.println(settingsHardware.hasO2Sensor);
        Serial.println(settingsHardware.O2RxPin);
        Serial.println(settingsHardware.O2TxPin);
        // Gas Relay
        Serial.println(settingsHardware.firstGasRelay);
        Serial.println(settingsHardware.gasRelayPin);
        Serial.println(settingsHardware.secondGasRelay);
        Serial.println(settingsHardware.gasRelayTwoPin);
        // PiLink
        Serial.println(settingsHardware.piSupport);
        Serial.println(settingsHardware.piRxPin);
        Serial.println(settingsHardware.piTxPin);
        // Lighting
        Serial.println(settingsHardware.lightingSupport);
        Serial.println(settingsHardware.lightPin);
        Serial.println(F("/RdHrdwrSet"));
        #endif
        return true;
      }
    }

    int ReadCurrentSettings() {
      #ifdef DEBUG_EEPROM
        Serial.println(F("ReadSettings"));
        Serial.print(F("\t"));
        Serial.print(sizeof(settingsHolder));
        Serial.print(F(" bytes w/ ID: "));
        Serial.print(SETTINGS_IDENT_CURR);
        Serial.print(F(" @ "));
        Serial.println(SETTINGS_ADDRS);
      #endif
      
      for (unsigned int i = 0; i < sizeof(this->settingsHolder); i++) {
        *((char*)&this->settingsHolder + i) = EEPROM.read(SETTINGS_ADDRS + i);    
      }
 
      #ifdef DEBUG_EEPROM
        Serial.println(settingsHolder.ident);
        Serial.println(settingsHolder.fanMode);
        Serial.println(settingsHolder.heatMode);
        Serial.println(settingsHolder.heatSetPoint);
        Serial.println(settingsHolder.CO2Mode);
        Serial.println(settingsHolder.CO2SetPoint);
        Serial.println(settingsHolder.O2Mode);
        Serial.println(settingsHolder.O2SetPoint);
        Serial.println(settingsHolder.lightMode);
        Serial.println(settingsHolder.millisOn);
        Serial.println(settingsHolder.millisOff);
        Serial.println(settingsHolder.alarmMode);
        
      #endif
      return 1;
    }
    
  public:
    
    // Defaults
    void ResetSettingsToDefaults()
    {
      #ifdef DEBUG_EEPROM
        Serial.println(F("Defaults"));
      #endif
      // Fan setup
      this->settingsHolder.fanMode = 4;
      // Heat setup
      this->settingsHolder.heatMode = 1;
      this->settingsHolder.heatSetPoint = TEMPERATURE_DEF; 
      // CO2 setup
      this->settingsHolder.CO2Mode = 2;
      this->settingsHolder.CO2SetPoint = CO2_DEF; 
      // O2 setup
      this->settingsHolder.O2Mode = 2;
      this->settingsHolder.O2SetPoint = OO_DEF; 
      // Lighting
      this->settingsHolder.lightMode = 0;
      this->settingsHolder.millisOn =  60000;  // 14 hrs = 50400000 milliseconds
      this->settingsHolder.millisOff = 30000;  // 10 hrs = 36000000 milliseconds
      // Alarm
      this->settingsHolder.alarmMode = 2;
      #ifdef DEBUG_EEPROM
        Serial.println(F("/Defaults"));
      #endif
    }
    
    void PerformSaveSettings() {
      #ifdef DEBUG_EEPROM
        Serial.println(F("SaveSettings"));
      #endif
    
      this->settingsHolder.ident = SETTINGS_IDENT_CURR;
    
      #ifdef DEBUG_EEPROM
        Serial.print(F("\tID: "));
        Serial.print(SETTINGS_IDENT_CURR);
        Serial.print(", ");
        Serial.print(sizeof(settingsHolder));
        Serial.print(" bytes");
      #endif
      
      for (unsigned int i=0; i < sizeof(settingsHolder); i++) {
        EEPROM.write(SETTINGS_ADDRS + i, *((char*)&this->settingsHolder + i));
        #ifdef DEBUG_EEPROM
          Serial.print(".");
        #endif
      }
      
      #ifdef DEBUG_EEPROM
        Serial.println("  Done.");
        Serial.println(F("/SaveSettings"));
      #endif
    }
    
    int PerformLoadSettings() {
      #ifdef DEBUG_EEPROM
        Serial.println(F("LoadSettings"));
      #endif
      int runMode = 0;
      
      if (ReadHardwareSettings()) {
        if (VerifyEEPROMHeader((int)SETTINGS_ADDRS, false) == SETTINGS_IDENT_CURR) {
          runMode = ReadCurrentSettings();
        } else {
          runMode = 2;
          #ifdef DEBUG_EEPROM
            Serial.println(F("\tNo settings found, using defaults."));
          #endif
          ResetSettingsToDefaults();
        }
      } 
    
      #ifdef DEBUG_EEPROM
        Serial.println(F("/LoadSettings"));
      #endif
      return runMode;
    }
    
    void CheckSettings() {
      this->personalityCount = 0;
      if (this->settingsHolder.heatMode == 1) {
        this->personalityCount++;
      }
      if (this->settingsHolder.lightMode > 0) {
        this->personalityCount++;
      }
      if (this->settingsHolder.CO2Mode > 0) {
        this->personalityCount++;
      }
      if (this->settingsHolder.O2Mode > 0) {
        this->personalityCount++;
      }
    
      #ifdef DEBUG_GENERAL
        Serial.print(F("Counted "));
        Serial.print(this->personalityCount);
        Serial.println(F(" personalities enabled."));
      #endif
    }

    void AttachIncuversModule(IncuversHeatingSystem* iHeat) {
      this->incHeat = iHeat;

      this->incHeat->SetupHeating(PINASSIGN_HEATDOOR, 
                      PINASSIGN_HEATCHAMBER, 
                      PINASSIGN_ONEWIRE_BUS, 
                      this->settingsHardware.sensorAddrDoorTemp,
                      this->settingsHardware.sensorAddrChamberTemp,
                      this->settingsHolder.heatMode,
                      PINASSIGN_FAN,
                      this->settingsHolder.fanMode,
                      this->settingsHolder.heatSetPoint);
    }

    IncuversHeatingSystem* getHeatModule() {
      return this->incHeat;
    }

    void AttachIncuversModule(IncuversLightingSystem* iLight) {
      this->incLight = iLight;

      this->incLight->SetupLighting(this->settingsHardware.lightPin,
                      this->settingsHardware.lightingSupport);
      this->incLight->UpdateLightDeltas(this->settingsHolder.millisOn, this->settingsHolder.millisOff);
    }

    IncuversLightingSystem* getLightModule() {
      return this->incLight;
    }

    void AttachIncuversModule(IncuversCO2System* iCO2) {
      this->incCO2 = iCO2;
      
      this->incCO2->SetupCO2(this->settingsHardware.CO2RxPin, 
                             this->settingsHardware.CO2TxPin,
                             this->settingsHardware.CO2RelayPin);
      this->incCO2->UpdateMode(this->settingsHolder.CO2Mode);                      
      this->incCO2->SetSetPoint(this->settingsHolder.CO2SetPoint);
    }

    IncuversCO2System* getCO2Module() {
      return this->incCO2;
    }

    void AttachIncuversModule(IncuversO2System* iO2) {
      this->incO2 = iO2;
      
      this->incO2->SetupO2(this->settingsHardware.O2RxPin, 
                           this->settingsHardware.O2TxPin,
                           this->settingsHardware.O2RelayPin);
      this->incO2->UpdateMode(this->settingsHolder.O2Mode);                      
      this->incO2->SetSetPoint(this->settingsHolder.O2SetPoint);
    }

    IncuversO2System* getO2Module() {
      return this->incO2;
    }

    int getPersonalityCount() {
      return personalityCount;
    }

    int getHeatMode() {
      return this->settingsHolder.heatMode;
    }

    void setHeatMode(int mode) {
      this->settingsHolder.heatMode = mode;
      this->incHeat->UpdateHeatMode(mode);
    }

    int getFanMode() {
      return this->settingsHolder.fanMode;
    }

    void setFanMode(int mode) {
      this->settingsHolder.fanMode = mode;
      this->incHeat->UpdateFanMode(mode);
    }
    
    float getDoorTemperature() {
      return incHeat->getDoorTemperature();
    }

    float getOtherTemperature() {
      return incHeat->getOtherTemperature();
    }
    
    boolean isDoorOn() {
      return incHeat->isDoorOn();
    }

    boolean isDoorStepping() {
      return incHeat->isDoorStepping();
    }

    float getChamberTemperature() {
      return incHeat->getChamberTemperature();
    }

    float getTemperatureSetPoint() {
      return this->settingsHolder.heatSetPoint;
    }

    void setTemperatureSetPoint(float newValue) {
      this->settingsHolder.heatSetPoint = newValue;
      this->incHeat->SetSetPoint(this->settingsHolder.heatSetPoint);
    }

    boolean isChamberOn() {
      return incHeat->isChamberOn();
    }

    boolean isChamberStepping() {
      return incHeat->isChamberStepping();
    }

    int getCO2Mode() {
      return this->settingsHolder.CO2Mode;
    }

    void setCO2Mode(int mode) {
      this->settingsHolder.CO2Mode = mode;
      this->incCO2->UpdateMode(mode);
    }
    
    float getCO2Level() {
      return incCO2->getCO2Level();
    }

    float getCO2SetPoint() {
      return this->settingsHolder.CO2SetPoint;
    }

    void setCO2SetPoint(float newValue) {
      this->settingsHolder.CO2SetPoint = newValue;
      this->incCO2->SetSetPoint(this->settingsHolder.CO2SetPoint);
    }

    boolean isCO2Open() {
      return incCO2->isCO2Open();
    }

    boolean isCO2Stepping() {
      return incCO2->isCO2Stepping();
    }

    int getO2Mode() {
      return this->settingsHolder.O2Mode;
    }

    void setO2Mode(int mode) {
      this->settingsHolder.O2Mode = mode;
      this->incO2->UpdateMode(mode);
    }
    
    float getO2Level() {
      return incO2->getO2Level();
    }

    float getO2SetPoint() {
      return this->settingsHolder.O2SetPoint;
    }

    void setO2SetPoint(float newValue) {
      this->settingsHolder.O2SetPoint = newValue;
      this->incO2->SetSetPoint(this->settingsHolder.O2SetPoint);
    }

    boolean isO2Open() {
      return incO2->isNOpen();
    }

    boolean isO2Stepping() {
      return incO2->isNStepping();
    }

    String getHardware() {
      return String(this->settingsHardware.hVer[0])+"."+String(this->settingsHardware.hVer[1])+"."+String(this->settingsHardware.hVer[2]);  
    }
    
    String getSerial() {
      return String(this->settingsHardware.serial);
    }

    void MakeSafeState() {
      incHeat->MakeSafeState();
      incLight->MakeSafeState();
      incCO2->MakeSafeState();
      incO2->MakeSafeState();
    }

    int getLightMode() {
      return this->settingsHolder.lightMode;
    }

    void setLightMode(int mode) {
      this->settingsHolder.lightMode = mode;
      this->incLight->UpdateMode(mode);
    }
    
    boolean HasCO2Sensor() {
      return settingsHardware.hasCO2Sensor;
    }

    boolean HasO2Sensor() {
      return settingsHardware.hasO2Sensor;
    }

    int CountGasRelays() {
      int count = 0;
      if (settingsHardware.CO2GasRelay) { count++; }
      if (settingsHardware.O2GasRelay) { count++; }
      return count;
    }

    boolean HasPiLink() {
      return settingsHardware.piSupport;
    }

    boolean HasLighting() {
      return settingsHardware.lightingSupport;
    }
        
    boolean isHeatAlarmed() {
      return incHeat->isAlarmed();
    }

    boolean isCO2Alarmed() {
      return incCO2->isAlarmed();
    }

    boolean isO2Alarmed() {
      return incO2->isAlarmed();
    }

    void ResetAlarms() {
      incHeat->ResetAlarms();
      incCO2->ResetAlarms();
      incO2->ResetAlarms();
    }
    
    int getAlarmMode() {
      return this->settingsHolder.alarmMode;
    }
    

};




