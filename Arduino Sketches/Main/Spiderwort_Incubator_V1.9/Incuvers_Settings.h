// Hardware settings definitions
#define HARDWARE_IDENT "IM1"
#define HARDWARE_ADDRS 4 

// Settings definitions
#define SETTINGS_IDENT_14 14
#define SETTINGS_IDENT_17 17
#define SETTINGS_IDENT_CURR 19
#define SETTINGS_ADDRS 64

// Defaults
#define TEMPERATURE_DEF 37.0
#define CO2_DEF 5.0
#define OO_DEF 5.0



// Structures
struct HardwareStruct {
  byte ident[3];
  // Identification
  byte hVer[3];
  byte serial;
  // Pin settings
  byte relayOnePin;
  byte relayTwoPin;
  byte sensorOneRxPin;
  byte sensorOneTxPin;
  byte sensorTwoRxPin;
  byte sensorTwoTxPin;
  // Installed Components
  byte countOfTempSensors;
  bool secondGasSensor;
  bool secondGasRelay;
  bool secondGasBidirectional;
  bool ethernetSupport;
  byte sensorAddrDoorTemp[8];
  byte sensorAddrChamberTemp[8];
};

struct HardwareDefinition {
  // Identification
  String identification;
  String serial;
  // Pin settings
  int relayOnePin;
  int relayTwoPin;
  int sensorOneRxPin;
  int sensorOneTxPin;
  int sensorTwoRxPin;
  int sensorTwoTxPin;
  int ethernetPin;
  // Installed Components
  int countOfTempSensors;
  bool secondGasSensor;
  bool secondGasRelay;
  bool secondGasBidirectional;
  bool ethernetSupport;
  byte sensorAddrDoorTemp[8];
  byte sensorAddrChamberTemp[8];
};

struct SettingsStruct_Curr {
  byte ident;
  // Fan settings
  byte fanMode;       // 0 = off, 1 = on during heat + 30 seconds after, 2 = on during heat + 60 seconds, 3 = on during heat + 50% of time, 4 = on
  // Temp settings
  byte heatMode;      // 0 = off, 1 = on
  float heatSetPoint;
  // CO2 settings
  byte CO2Mode;       // 0 = off, all other numbers indicates sensorID
  byte CO2Relay;      // 0 = off, all other numbers indicates relayID
  float CO2SetPoint;
  // O2 settings
  byte O2Mode;        // 0 = off, all other numbers indicates sensorID
  byte O2Relay;       // 0 = off, all other numbers indicates relayID
  float O2SetPoint;
};

struct SettingsStruct_17 {
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

class IncuversSettingsHandler {
  private:
    HardwareDefinition settingsHardware;
    SettingsStruct_Curr settingsHolder;
  
    IncuversHeatingSystem* incHeat;
    IncuversCO2System* incCO2;
    IncuversO2System* incO2;
    
    int personalityCount;
    
    
    int VerifyEEPROMHeader(int startAddress, boolean isHardware) {
      #ifdef DEBUG_EEPROM
        Serial.println(F("  Call to IncuversSettingsHandler::VerifyHeader()"));
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
        if (eepromContent[0] == SETTINGS_IDENT_14 || eepromContent[0] == SETTINGS_IDENT_17 ||  eepromContent[0] == SETTINGS_IDENT_CURR) {
          ret = (int)eepromContent[0]; 
        } else {
          ret = -1;
        }
      }
      
      #ifdef DEBUG_EEPROM
        Serial.print(F("  /VerifyHeader(); returning: "));
        Serial.println(ret);
      #endif
      
      return ret;
    }

    boolean ReadHardwareSettings() {
      #ifdef DEBUG_EEPROM
        Serial.println(F("Call to IncuversSettingsHandler::ReadHardwareSettings()"));
      #endif
      int headerCheck;
      HardwareStruct hardwareData;
      
      headerCheck = VerifyEEPROMHeader(HARDWARE_ADDRS, true);
    
      if (headerCheck == -1) {
        // uninitialized hardware
        #ifdef DEBUG_EEPROM
          Serial.println(F("  The hardware in this incubator has not been initialized, defaulting to original specs."));
        #endif
        
        this->settingsHardware.identification = F("0.9.3");
        this->settingsHardware.serial = F("XXXX");
        this->settingsHardware.relayOnePin = PINASSIGN_CO2RELAY;
        this->settingsHardware.relayTwoPin = PINASSIGN_NRELAY;
        this->settingsHardware.sensorOneRxPin = PINASSIGN_CO2SENSOR_RX;
        this->settingsHardware.sensorOneTxPin = PINASSIGN_CO2SENSOR_TX;
        this->settingsHardware.sensorTwoRxPin = PINASSIGN_O2SENSOR_RX;
        this->settingsHardware.sensorTwoTxPin = PINASSIGN_O2SENSOR_TX;
        this->settingsHardware.ethernetPin = 10;
        this->settingsHardware.countOfTempSensors = 2;
        this->settingsHardware.secondGasSensor = false;
        this->settingsHardware.secondGasRelay = false;
        this->settingsHardware.secondGasBidirectional = false;
        this->settingsHardware.ethernetSupport = false;
        for (int i = 0; i < 8; i++) {
          this->settingsHardware.sensorAddrDoorTemp[i] = 0;
          this->settingsHardware.sensorAddrChamberTemp[i] = 0;
        }
      } else {
        #ifdef DEBUG_EEPROM
          Serial.print(F("  The hardwareData ident matched, loading "));
          Serial.print(sizeof(hardwareData));
          Serial.println(F(" bytes of hardware definitions."));
        #endif
        
        for (int i = 0; i < sizeof(hardwareData); i++) {
          *((char*)&hardwareData + i) = EEPROM.read(HARDWARE_ADDRS + i);    
        }
    
        this->settingsHardware.identification = String(hardwareData.hVer[0]) + "." + String(hardwareData.hVer[1]) + "." + String(hardwareData.hVer[2]);
        this->settingsHardware.serial = String(hardwareData.serial);
        this->settingsHardware.relayOnePin = (int)hardwareData.relayOnePin;
        this->settingsHardware.relayTwoPin = (int)hardwareData.relayTwoPin;
        this->settingsHardware.sensorOneRxPin = (int)hardwareData.sensorOneRxPin;
        this->settingsHardware.sensorOneTxPin = (int)hardwareData.sensorOneTxPin;
        this->settingsHardware.sensorTwoRxPin = (int)hardwareData.sensorTwoRxPin;
        this->settingsHardware.sensorTwoTxPin = (int)hardwareData.sensorTwoTxPin;
        this->settingsHardware.ethernetPin = 10;
        this->settingsHardware.secondGasSensor = (bool)hardwareData.secondGasSensor;
        this->settingsHardware.secondGasRelay = (bool)hardwareData.secondGasRelay;
        this->settingsHardware.secondGasBidirectional = (bool)hardwareData.secondGasBidirectional;
        this->settingsHardware.ethernetSupport = (bool)hardwareData.ethernetSupport;
        for (int i = 0; i < 8; i++) {
          this->settingsHardware.sensorAddrDoorTemp[i] = hardwareData.sensorAddrDoorTemp[i];
          this->settingsHardware.sensorAddrChamberTemp[i] = hardwareData.sensorAddrChamberTemp[i];
        }
      }
    
      #ifdef DEBUG_EEPROM
        Serial.println(F("Return from ReadHardwareSettings()"));
      #endif
      return true;
    }

    void ImportSettings_17() {
      SettingsStruct_17 settingsHolder_17;
      #ifdef DEBUG_EEPROM
        Serial.println(F("Call to ImportantSettings_17()"));
        Serial.print(F("  "));
        Serial.print(sizeof(settingsHolder_17));
        Serial.print(F(" bytes w/ ID: "));
        Serial.print(SETTINGS_IDENT_17);
        Serial.print(F(" @ "));
        Serial.println(SETTINGS_ADDRS);
      #endif
      for (unsigned int i = 0; i < sizeof(settingsHolder_17); i++) {
        *((char*)&settingsHolder_17 + i) = EEPROM.read(SETTINGS_ADDRS + i);    
      }
      ResetSettingsToDefaults();
    
      if (settingsHolder_17.heatEnable) {
        this->settingsHolder.heatMode = 1;
      } else {
        this->settingsHolder.heatMode = 0;
      }
      this->settingsHolder.heatSetPoint = settingsHolder_17.heatSetPoint;
      if (settingsHolder_17.CO2Enable) {
        this->settingsHolder.CO2Mode = 1;
      } else {
        this->settingsHolder.CO2Mode = 0;
      }
      this->settingsHolder.CO2SetPoint = settingsHolder_17.CO2SetPoint;
      if (settingsHolder_17.O2Enable) {
        if (settingsHolder_17.CO2Enable) {
          this->settingsHolder.O2Mode = 2;
        } else {
          this->settingsHolder.O2Mode = 1;
        }
      } else {
        this->settingsHolder.O2Mode = 0;
      }
      this->settingsHolder.O2SetPoint = settingsHolder_17.O2SetPoint;
      
      #ifdef DEBUG_EEPROM
        Serial.println(F("/ImportSettings_17()"));
      #endif
    }
    
    void ImportSettings_14() {
      SettingsStruct_14 settingsHolder_14;
      #ifdef DEBUG_EEPROM
        Serial.println(F("Call to ImportantSettings_14()"));
        Serial.print(F("  "));
        Serial.print(sizeof(settingsHolder_14));
        Serial.print(F(" bytes w/ ID: "));
        Serial.print(SETTINGS_IDENT_14);
        Serial.print(F(" @ "));
        Serial.println(SETTINGS_ADDRS);
      #endif
      for (unsigned int i = 0; i < sizeof(settingsHolder_14); i++) {
        *((char*)&settingsHolder_14 + i) = EEPROM.read(SETTINGS_ADDRS + i);    
      }
      ResetSettingsToDefaults();
    
      if (settingsHolder_14.heatEnable) {
        this->settingsHolder.heatMode = 1;
      } else {
        this->settingsHolder.heatMode = 0;
      }
      this->settingsHolder.heatSetPoint = settingsHolder_14.heatSetPoint;
      if (settingsHolder_14.CO2Enable) {
        this->settingsHolder.CO2Mode = 1;
      } else {
        this->settingsHolder.CO2Mode = 0;
      }
      this->settingsHolder.CO2SetPoint = settingsHolder_14.CO2SetPoint;
      
      #ifdef DEBUG_EEPROM
        Serial.println(F("End of ImportSettings_14()"));
      #endif
    }

    int ReadCurrentSettings() {
      #ifdef DEBUG_EEPROM
        Serial.println(F("Call to ReadCurrentSettings()"));
        Serial.print(F("  "));
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
        Serial.println(F("End of ReadCurrentSettings()"));
      #endif
      return 1;
    }
    
  public:
    
    // Defaults
    void ResetSettingsToDefaults()
    {
      #ifdef DEBUG_EEPROM
        Serial.println(F("Call to ResetSettingsToDefaults()"));
      #endif
      // Fan setup
      this->settingsHolder.fanMode = 4;
      // Heat setup
      this->settingsHolder.heatMode = 0;
      this->settingsHolder.heatSetPoint = TEMPERATURE_DEF; 
      // CO2 setup
      this->settingsHolder.CO2Mode = 1;
      this->settingsHolder.CO2Relay = 1;
      this->settingsHolder.CO2SetPoint = CO2_DEF; 
      // O2 setup
      this->settingsHolder.O2Mode = 0;
      this->settingsHolder.O2Relay = 0;
      this->settingsHolder.O2SetPoint = OO_DEF; 
      #ifdef DEBUG_EEPROM
        Serial.println(F("Done ResetSettingsToDefaults()"));
      #endif
    }
    
    void PerformSaveSettings() {
      #ifdef DEBUG_EEPROM
        Serial.println(F("Call to PerformSaveSettings()"));
      #endif
    
      this->settingsHolder.ident = SETTINGS_IDENT_CURR;
    
      #ifdef DEBUG_EEPROM
        Serial.print(F("  ID: "));
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
        Serial.println(F("End of PerformSaveSettings()"));
      #endif
    }
    
    int PerformLoadSettings() {
      #ifdef DEBUG_EEPROM
        Serial.println(F("Call to PerformLoadSettings()"));
      #endif
      int runMode = 0;
      bool hardwareResult = ReadHardwareSettings();
      int settingsRes = VerifyEEPROMHeader((int)SETTINGS_ADDRS, false);
    
      if (settingsRes == -1) {
        runMode = 2;
        #ifdef DEBUG_EEPROM
          Serial.println(F("  The settings for this incubator have not been saved, using defaults."));
        #endif
        ResetSettingsToDefaults();
      } else {
        switch (settingsRes) {
          case SETTINGS_IDENT_14:
            ImportSettings_14();
            runMode = 4;
            break;
          case SETTINGS_IDENT_17:
            ImportSettings_17();
            runMode = 4;
            break;
          case SETTINGS_IDENT_CURR:
            runMode = ReadCurrentSettings();
            break;
        }
      }
    
      #ifdef DEBUG_EEPROM
        Serial.println(F("End of PerformLoadSettings()"));
      #endif
      return runMode;
    }
    
    void CheckSettings() {
      this->personalityCount = 0;
      if (this->settingsHolder.heatMode == 1) {
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
    
        // Temp Settings
        #ifdef DEBUG_EEPROM
          Serial.print(F("Heating Enabled: "));
          Serial.println();
          Serial.print(F("Temperature setpoint: "));
          Serial.println(this->settingsHolder.heatSetPoint);
          Serial.println();
          Serial.println(F("Settings retrieved."));
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
                      this->settingsHolder.fanMode);
      this->incHeat->SetSetPoint(this->settingsHolder.heatSetPoint);
    }

    void AttachIncuversModule(IncuversCO2System* iCO2) {
      this->incCO2 = iCO2;
      
      if (this->settingsHolder.CO2Mode == 1) {
        this->incCO2->SetupCO2(this->settingsHardware.sensorOneRxPin, 
                        this->settingsHardware.sensorOneTxPin, 
                        this->settingsHolder.CO2Mode, 
                        this->settingsHardware.relayOnePin);
        this->incCO2->SetSetPoint(this->settingsHolder.CO2SetPoint);
      } else if (this->settingsHolder.CO2Mode == 2) {
        this->incCO2->SetupCO2(this->settingsHardware.sensorTwoRxPin, 
                        this->settingsHardware.sensorTwoTxPin, 
                        this->settingsHolder.CO2Mode, 
                        this->settingsHardware.relayTwoPin);
        this->incCO2->SetSetPoint(this->settingsHolder.CO2SetPoint);
      } else {
        this->incCO2->SetupCO2(A0, 
                        A0, 
                        this->settingsHolder.CO2Mode, 
                        A0);
      }
    }

    void AttachIncuversModule(IncuversO2System* iO2) {
      this->incO2 = iO2;
      
      if (this->settingsHolder.O2Mode == 1) {
        this->incO2->SetupO2(this->settingsHardware.sensorOneRxPin, 
                        this->settingsHardware.sensorOneTxPin, 
                        this->settingsHolder.O2Mode, 
                        this->settingsHardware.relayOnePin);
        this->incO2->SetSetPoint(this->settingsHolder.O2SetPoint);
      } else if (this->settingsHolder.O2Mode == 2) {
        this->incO2->SetupO2(this->settingsHardware.sensorTwoRxPin, 
                        this->settingsHardware.sensorTwoTxPin, 
                        this->settingsHolder.O2Mode, 
                        this->settingsHardware.relayTwoPin);
        this->incO2->SetSetPoint(this->settingsHolder.O2SetPoint);
      } else {
        this->incO2->SetupO2(A0, 
                        A0, 
                        this->settingsHolder.O2Mode, 
                        A0);
      } 
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

    boolean isSecondGasSensorSupported() {
      return this->settingsHardware.secondGasSensor;
    }

    String getSerial() {
      return this->settingsHardware.serial;
    }

    void MakeSafeState() {
      incHeat->MakeSafeState();
      incCO2->MakeSafeState();
      incO2->MakeSafeState();
    }
};



