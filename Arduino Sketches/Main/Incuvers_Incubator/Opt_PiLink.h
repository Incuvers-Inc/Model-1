#ifdef INCLUDE_PILINK 
/*
 * Incuvers PiLink code.
 * 
 * Only functional on 1.0.0+ control board units.
 */
class IncuversPiLink {
  private:
    IncuversSettingsHandler* incSet;
    bool isEnabled;

    void CheckForCommands() {
      
    }
    
    void SendStatus() {
      // General Identification
      Serial1.print(millis());
      Serial1.print(F(" ID "));              // Identification
      Serial1.print(incSet->getSerial());
      // Heating/Fan system
      Serial1.print(F(" FM "));              // Fan, mode
      Serial1.print(incSet->getFanMode());
      Serial1.print(F(" TM "));              // Temperature, mode
      Serial1.print(incSet->getHeatMode());
      Serial1.print(F(" TP "));              // Temperature, setpoint
      Serial1.print(incSet->getTemperatureSetPoint(), 2);
      Serial1.print(F(" TC "));              // Temperature, chamber
      Serial1.print(incSet->getChamberTemperature(), 2);
      Serial1.print(F(" TD "));              // Temperature, door
      Serial1.print(incSet->getDoorTemperature(), 2);
      Serial1.print(F(" TS "));              // Temperature, status
      Serial1.print(GetIndicator(incSet->isDoorOn(), incSet->isDoorStepping(), false, true));
      Serial1.print(GetIndicator(incSet->isChamberOn(), incSet->isChamberStepping(), false, true));
      Serial1.print(F(" TA "));              // Temperature, alarms
      Serial1.print(GetIndicator(incSet->isHeatAlarmed(), false, false, true));
      // CO2 system
      Serial1.print(F(" CM "));              // CO2, mode
      Serial1.print(incSet->getCO2Mode());
      Serial1.print(F(" CP "));              // CO2, setpoint
      Serial1.print(incSet->getCO2SetPoint(), 2);
      Serial1.print(F(" CC "));              // CO2, reading
      Serial1.print(incSet->getCO2Level(), 2);
      Serial1.print(F(" CS "));              // CO2, status
      Serial1.print(GetIndicator(incSet->isCO2Open(), incSet->isCO2Stepping(), false, true));
      Serial1.print(F(" CA "));              // CO2, alarms
      Serial1.print(GetIndicator(incSet->isCO2Alarmed(), false, false, true));
      // O2 system
      Serial1.print(F(" OM "));              // O2, mode
      Serial1.print(incSet->getO2Mode());
      Serial1.print(F(" OP "));              // O2, setpoint
      Serial1.print(incSet->getO2SetPoint(), 2);
      Serial1.print(F(" OC "));              // O2, reading
      Serial1.print(incSet->getO2Level(), 2);
      Serial1.print(F(" OS "));              // CO2, status
      Serial1.print(GetIndicator(incSet->isO2Open(), incSet->isO2Stepping(), false, true));
      Serial1.print(F(" OA "));              // CO2, alarms
      Serial1.print(GetIndicator(incSet->isO2Alarmed(), false, false, true));
      // Options
      Serial1.print(F(" LM "));              // Light Mode
      Serial1.print(incSet->getLightMode());
      Serial1.print(F(" LS "));              // Light System
      Serial1.print(incSet->getLightModule()->GetSerialAPIndicator());
      // Debugging
      Serial1.print(F(" FM "));              // Free memory
      Serial1.print(freeMemory());
      Serial1.println();
    }
    
  public:
    void SetupPiLink(IncuversSettingsHandler* iSettings) {
      this->incSet = iSettings;
      if (this->incSet->HasPiLink()) {
        Serial1.begin(9600, SERIAL_8E2);
      }
    }

    void DoTick() {
      if (this->incSet->HasPiLink()) {
        CheckForCommands();
        SendStatus();
      }
    }

};

#else
class IncuversPiLink {
  public:
    void SetupPiLink(IncuversSettingsHandler* iSettings) {
    }

    void DoTick() {
    }

};
#endif
