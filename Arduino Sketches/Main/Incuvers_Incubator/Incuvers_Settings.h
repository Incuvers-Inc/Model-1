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

struct VolatileValuesStruct {
  bool wifiMACSet;
  byte wifiMAC[18];
  bool wiredMACSet;
  byte wiredMAC[18];
  bool ipV4Set;
  byte activeV4IP[16];
  bool ipV6Set;
  byte activeV6IP[42];
  byte piSerial[18];
  byte valuesVersion;  
};

class IncuversSettingsHandler {
  private:
    HardwareStruct settingsHardware;
    SettingsStruct settingsHolder;
    VolatileValuesStruct settingsRuntime;
  
    IncuversHeatingSystem* incHeat;
    IncuversLightingSystem* incLight;
    IncuversCO2System* incCO2;
    IncuversO2System* incO2;
    
    int personalityCount;

    void InitializeRuntimeSettings() {
      strcpy_P(this->settingsRuntime.wifiMAC, (const char *) F("000000000000"));  
      this->settingsRuntime.wifiMACSet = false;
      strcpy_P(this->settingsRuntime.wiredMAC, (const char *) F("000000000000"));  
      this->settingsRuntime.wiredMACSet = false;
      strcpy_P(this->settingsRuntime.activeV4IP, (const char *) F("0.0.0.0"));
      this->settingsRuntime.ipV4Set = false;
      strcpy_P(this->settingsRuntime.activeV6IP, (const char *) F("0::0"));
      this->settingsRuntime.ipV6Set = false;
      strcpy_P(this->settingsRuntime.piSerial, (const char *) F("0"));
      this->settingsRuntime.valuesVersion = 0;  
    }
    
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

      this->InitializeRuntimeSettings();
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

    String GenerateStatusLine(bool includeCRC)
    {
      // General Identification
      String piLink = String("TS|" + String(millis(), DEC));
      piLink = String(piLink + F("&ID|") + getSerial());             // Ident rPi Serial
      piLink = String(piLink + F("&IV|") + F(SOFTWARE_VER_STRING));                             // Ident Software Version
      piLink = String(piLink + F("&IR|") + String(this->settingsRuntime.valuesVersion));        // Ident Runtime Settings Version
      // Heating/Fan system
      piLink = String(piLink + F("&FM|") + this->settingsHolder.fanMode);                       // Fan, mode
      piLink = String(piLink + F("&TM|") + String(this->settingsHolder.heatMode, DEC));         // Temperature, mode
      piLink = String(piLink + F("&TP|") + String(this->settingsHolder.heatSetPoint * 100, 0)); // Temperature, setpoint
      piLink = String(piLink + F("&TC|") + String(incHeat->getChamberTemperature() * 100, 0));  // Temperature, chamber
      piLink = String(piLink + F("&TD|") + String(incHeat->getDoorTemperature() * 100, 0));     // Temperature, door
      piLink = String(piLink + F("&TO|") + String(incHeat->getOtherTemperature() * 100, 0));    // Temperature, other
      piLink = String(piLink + F("&TS|") + String(GetIndicator(incHeat->isDoorOn(), incHeat->isDoorStepping(), false, true)) + String(GetIndicator(incHeat->isChamberOn(), incHeat->isChamberStepping(), false, true)));  // Temperature, status
      piLink = String(piLink + F("&TA|") + String(GetIndicator(incHeat->isAlarmed(), false, false, true)));                                                                                                           // Temperature, alarms
      // CO2 system
      piLink = String(piLink + F("&CM|") + String(this->settingsHolder.CO2Mode, DEC));          // CO2, mode
      piLink = String(piLink + F("&CP|") + String(this->settingsHolder.CO2SetPoint * 100, 0));  // CO2, setpoint
      piLink = String(piLink + F("&CC|") + String(incCO2->getCO2Level() * 100, 0));             // CO2, reading
      piLink = String(piLink + F("&CS|") + GetIndicator(incCO2->isCO2Open(), incCO2->isCO2Stepping(), false, true));  // CO2, status
      piLink = String(piLink + F("&CA|") + GetIndicator(incCO2->isAlarmed(), false, false, true));                 // CO2, alarms
      // O2 system
      piLink = String(piLink + F("&OM|") + String(this->settingsHolder.O2Mode, DEC));           // O2, mode
      piLink = String(piLink + F("&OP|") + String(this->settingsHolder.O2SetPoint * 100, 0));   // O2, setpoint
      piLink = String(piLink + F("&OC|") + String(incO2->getO2Level() * 100, 0));               // O2, reading
      piLink = String(piLink + F("&OS|") + GetIndicator(incO2->isNOpen(), incO2->isNStepping(), false, true));  // CO2, status
      piLink = String(piLink + F("&OA|") + GetIndicator(incO2->isAlarmed(), false, false, true));               // CO2, alarms
      // Options
      piLink = String(piLink + F("&LM|") + String(this->settingsHolder.lightMode, DEC));        // Light Mode
      piLink = String(piLink + F("&LS|") + incLight->GetSerialAPIndicator());                   // Light System
      // Debugging
      piLink = String(piLink + F("&FM|") + String(freeMemory(), DEC));                          // Free memory

      if (includeCRC) {
        CRC32 crc;
        
        for (int i = 0; i < piLink.length(); i++) {
          crc.update(piLink[i]);
        }
  
        piLink = String(String(piLink.length(), DEC) + F("~") + PadHexToLen(String(crc.finalize(), HEX), 8) + F("$") + piLink);   // CRC to detect corrupted entries
      }
 
      return piLink;
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
    
    float getTemperatureSetPoint() {
      return this->settingsHolder.heatSetPoint;
    }

    void setTemperatureSetPoint(float newValue) {
      this->settingsHolder.heatSetPoint = newValue;
      this->incHeat->SetSetPoint(this->settingsHolder.heatSetPoint);
    }

    int getCO2Mode() {
      return this->settingsHolder.CO2Mode;
    }

    void setCO2Mode(int mode) {
      this->settingsHolder.CO2Mode = mode;
      this->incCO2->UpdateMode(mode);
    }
    
    float getCO2SetPoint() {
      return this->settingsHolder.CO2SetPoint;
    }

    void setCO2SetPoint(float newValue) {
      this->settingsHolder.CO2SetPoint = newValue;
      this->incCO2->SetSetPoint(this->settingsHolder.CO2SetPoint);
    }

    int getO2Mode() {
      return this->settingsHolder.O2Mode;
    }

    void setO2Mode(int mode) {
      this->settingsHolder.O2Mode = mode;
      this->incO2->UpdateMode(mode);
    }
    
    float getO2SetPoint() {
      return this->settingsHolder.O2SetPoint;
    }

    void setO2SetPoint(float newValue) {
      this->settingsHolder.O2SetPoint = newValue;
      this->incO2->SetSetPoint(this->settingsHolder.O2SetPoint);
    }

    String getHardware() {
      return String(this->settingsHardware.hVer[0])+"."+String(this->settingsHardware.hVer[1])+"."+String(this->settingsHardware.hVer[2]);  
    }
    
    String getSerial() {
      String serial = String( (char *) this->settingsRuntime.piSerial);
      serial.trim();
      return serial;
    }

    void setSerial(byte serial[]) {
      strcpy(this->settingsRuntime.piSerial, (char *) serial);
      this->settingsRuntime.valuesVersion++;
    }

    String getRuntimeValuesVersion() {
      return String(this->settingsRuntime.valuesVersion);
    }

    String getIP4() {
      if (this->settingsRuntime.ipV4Set) {
        String ip4 = String( (char *) this->settingsRuntime.activeV4IP);
        ip4.trim();
        return ip4;
      } else {
        return F("Not set");
      }
    }

    void setIP4(byte ip[]) {
      strcpy(this->settingsRuntime.activeV4IP, (char *) ip);
      this->settingsRuntime.ipV4Set = true;
      this->settingsRuntime.valuesVersion++;
    }
    
    String getWireMAC() {
      if (this->settingsRuntime.wiredMACSet) {
        String wireMAC = String( (char *) this->settingsRuntime.wiredMAC);
        wireMAC.trim();
        return wireMAC;
      } else {
        return F("Not set");
      }
    }

    void setWireMAC(byte mac[]) {
      strcpy(this->settingsRuntime.wiredMAC, (char *) mac);
      this->settingsRuntime.wiredMACSet = true;
      this->settingsRuntime.valuesVersion++;
    }
    
    String getWifiMAC() {
      if (this->settingsRuntime.wifiMACSet) {
        String wifiMAC = String( (char *) this->settingsRuntime.wifiMAC);
        wifiMAC.trim();
        return wifiMAC;
      } else {
        return F("Not set");
      }
    }

    void setWifiMAC(byte mac[]) {
      strcpy(this->settingsRuntime.wifiMAC, (char *) mac);
      this->settingsRuntime.wifiMACSet = true;
      this->settingsRuntime.valuesVersion++;
    }
       
    void MakeSafeState() {
      incHeat->MakeSafeState();
      incLight->MakeSafeState();
      incCO2->MakeSafeState();
      incO2->MakeSafeState();
    }

    void ReturnFromSafeState() {
      incHeat->ResumeState(this->settingsHolder.heatMode);
      // Lighting will resume automatically
      // CO2 doesn't use EM which requires a restart signal
      // O2 doesn't use EM which requires a restart signal
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
        
    void ResetAlarms() {
      incHeat->ResetAlarms();
      incCO2->ResetAlarms();
      incO2->ResetAlarms();
    }
    
    int getAlarmMode() {
      return this->settingsHolder.alarmMode;
    }
    

};
