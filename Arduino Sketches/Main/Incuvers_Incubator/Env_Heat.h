#define TEMPERATURE_STEP_LEN 1200
#define TEMPERATURE_JUMP_LEN 60000
#define TEMPERATURE_JUMP_WT 60000
#define TEMP_ALARM_THRESH 114.0
#define TEMP_ALARM_ON_PERIOD 7200000

class IncuversHeatingSystem {
  private:
    int pinAssignment_Fan;
    int fanMode;
    
    float tempDoor;
    float tempChamber;
    float tempOther;

    boolean oneTempSensorInterface;
    boolean tempSensorAux;
    boolean tempSensorChamber;
    boolean tempSensorDoor;

    bool otherTempSensorPresent;
    
    byte sensorAddrDoorTemp[8];
    byte sensorAddrChamberTemp[8];
    byte sensorAddrOtherTemp[8];
    
    IncuversEM EMHandleDoor;
    IncuversEM EMHandleChamber;

    OneWire* oneWireA;
    OneWire* oneWireC;
    OneWire* oneWireD;
    DallasTemperature* tempSensorsA;
    DallasTemperature* tempSensorsC;
    DallasTemperature* tempSensorsD;


    bool AreSensorsSame(byte addrA[], byte addrB[]) {
      bool result = true;
      for (int i = 0; i < 8; i++) {
        #ifdef DEBUG_TEMP
        Serial.print(addrA[i]);
        Serial.print(" to ");
        Serial.println(addrB[i]);
        #endif
        if (addrA[i] != addrB[i]) {
          result = false;  
        }
      }
      
      return result;
    }
    
    void CheckForOtherTempSonsors() {
      int i, j, k;
      byte addr[8];
      bool allocated;
      j=0;
      k=0;
      
      this->otherTempSensorPresent = false;
      
      while(oneWireA->search(addr)) {
        #ifdef DEBUG_TEMP
        Serial.print(addr[0]);
        Serial.print(addr[1]);
        Serial.print(addr[2]);
        Serial.print(addr[3]);
        Serial.print(addr[4]);
        Serial.print(addr[5]);
        Serial.print(addr[6]);
        Serial.print(addr[7]);
        #endif
        if (!AreSensorsSame(addr, sensorAddrDoorTemp) && !AreSensorsSame(addr, sensorAddrChamberTemp)) {
          Serial.println(" :: No match");
          k++;
          for( i = 0; i < 8; i++) {
            sensorAddrOtherTemp[i] = addr[i]; 
            this->otherTempSensorPresent = true;
          }
        }
        j++;
      }

      #ifdef DEBUG_TEMP
        Serial.print(F("Heat::CFOTS - Found "));
        Serial.print(j);
        Serial.print(F(" sensors, "));
        Serial.print(k);
        Serial.println(F(" weren't allocated."));
        Serial.println(F("\tAn address was added as the other temp sensor."));
      #endif
    }

    
    void GetTemperatureReadings() {
      #ifdef DEBUG_TEMP
        Serial.println(F("Heat::GetTempRead"));
      #endif
      boolean updateCompleted = false;
      int i = 0;
      float tD, tC, tO;
      
      while (!updateCompleted) {
        // Request the temperatures
        this->tempSensorsA->requestTemperatures();
        // Record the values
        tD = this->tempSensorsA->getTempC(this->sensorAddrDoorTemp);
        tC = this->tempSensorsA->getTempC(this->sensorAddrChamberTemp);
        if (this->otherTempSensorPresent) {
          tO = this->tempSensorsA->getTempC(this->sensorAddrOtherTemp);
        }
        
        #ifdef DEBUG_TEMP
          Serial.print(F("Door: "));
          Serial.print(tD);
          Serial.print(F("*C  Chamber: "));
          Serial.print(tC);
          Serial.println("*C");
        #endif  

        if (this->otherTempSensorPresent && tO > -40.0 && tO < 85.0 ) {
          this->tempOther = tO;
        }
        if (tD > -40.0 && tD < 85.0 ) {
          this->tempDoor = tD;
        }
        
        if (tC > -40.0 && tC < 85.0 ) {
          this->tempChamber = tC;
          updateCompleted = true;
        } else {
          i++;
          #ifdef DEBUG_TEMP
            Serial.print(F("Temperature sensors returned invalid reading"));
            Serial.println(i);
          #endif 
          if (i > 5) {
            //statusHolder.AlarmTempSensorMalfunction = true;
            updateCompleted = true;
          }
        }
      }
    }
  
    void GetNewTemperatureReadings() {
      #ifdef DEBUG_TEMP
        Serial.println(F("Heat::GetNewTempRead"));
      #endif
      boolean updateCompleted = false;
      int i = 0;
      float tD, tC, tO;
      
      while (!updateCompleted) {
        // Request the temperatures
        if (this->tempSensorAux) {
          this->tempSensorsA->requestTemperatures();
        }
        if (this->tempSensorChamber) {
          this->tempSensorsC->requestTemperatures();
        }
        if (this->tempSensorDoor) {
          this->tempSensorsD->requestTemperatures();
        }
        // Record the values
        if (this->tempSensorDoor) {
          tD = this->tempSensorsD->getTempCByIndex(0);
        }
        if (this->tempSensorChamber) {
          tC = this->tempSensorsC->getTempCByIndex(0);
        }
        if (this->tempSensorAux) {
          tO = this->tempSensorsA->getTempCByIndex(0);
        }
        
        #ifdef DEBUG_TEMP
          Serial.print(F("Door: "));
          Serial.print(tD);
          Serial.print(F("*C  Chamber: "));
          Serial.print(tC);
          Serial.println("*C");
        #endif  

        if (this->tempSensorAux && tO > -40.0 && tO < 85.0 ) {
          this->tempOther = tO;
        }
        if (tD > -40.0 && tD < 85.0 ) {
          this->tempDoor = tD;
        }
        
        if (tC > -40.0 && tC < 85.0 ) {
          this->tempChamber = tC;
          updateCompleted = true;
        } else {
          i++;
          #ifdef DEBUG_TEMP
            Serial.print(F("Temperature sensors returned invalid reading"));
            Serial.println(i);
          #endif 
          if (i > 5) {
            //statusHolder.AlarmTempSensorMalfunction = true;
            updateCompleted = true;
          }
        }
      }
    }
    
  public:
    void SetupHeating(int doorPin, int chamberPin, int oneWirePin, byte doorSensorID[8], byte chamberSensorID[8], int heatMode, int fanPin, int fanMode, float tempSetPoint) {
      #ifdef DEBUG_TEMP
        Serial.println(F("Heat::Setup"));
        Serial.println(doorPin);
        Serial.println(chamberPin);
        Serial.print(doorSensorID[0]);
        Serial.print(doorSensorID[1]);
        Serial.print(doorSensorID[2]);
        Serial.print(doorSensorID[3]);
        Serial.print(doorSensorID[4]);
        Serial.print(doorSensorID[5]);
        Serial.print(doorSensorID[6]);
        Serial.println(doorSensorID[7]);
        Serial.print(chamberSensorID[0]);
        Serial.print(chamberSensorID[1]);
        Serial.print(chamberSensorID[2]);
        Serial.print(chamberSensorID[3]);
        Serial.print(chamberSensorID[4]);
        Serial.print(chamberSensorID[5]);
        Serial.print(chamberSensorID[6]);
        Serial.println(chamberSensorID[7]);
        Serial.println(heatMode);
        Serial.println(fanPin);
        Serial.println(fanMode);
      #endif

      // OldSchool hardware
      this->oneTempSensorInterface = true;
      
      // Setup EMs
      this->EMHandleChamber.SetupEM(char('C'), true, tempSetPoint, 0, chamberPin);
      this->EMHandleChamber.SetupEM_Timing(false, TEMP_ALARM_ON_PERIOD, 90.0, true, false, TEMPERATURE_STEP_LEN, false, 0.0);
      this->EMHandleChamber.setupEM_Alarms(true, TEMP_ALARM_THRESH, true, TEMP_ALARM_ON_PERIOD);
      this->EMHandleDoor.SetupEM(char('D'), true, tempSetPoint, 0, doorPin);
      this->EMHandleDoor.SetupEM_Timing(false, TEMP_ALARM_ON_PERIOD, 90.0, true, false, TEMPERATURE_STEP_LEN, false, 0.0);
      this->EMHandleDoor.setupEM_Alarms(true, TEMP_ALARM_THRESH, true, RESET_AFTER_DELTA);  // We have a really long alarm period for the door as we aren't as concerned if it never reaches its destination temperature
      

      // MakeSafe
      this->MakeSafeState();

      for (int i = 0; i < 8; i++) {
        this->sensorAddrDoorTemp[i] = doorSensorID[i];
        this->sensorAddrChamberTemp[i] = chamberSensorID[i];
      }
      
      // Setup Temperature sensors
      this->oneWireA = new OneWire(PINASSIGN_ONEWIRE_BUS);       // Setup a oneWireA instance to communicate with ANY oneWireA devices
      this->tempSensorsA = new DallasTemperature(this->oneWireA);          // Pass our oneWireA reference to Dallas Temperature.

      this->CheckForOtherTempSonsors();
      tempOther = -100;
      
      // Starting the temperature sensors
      this->tempSensorsA->begin();
    
      if (heatMode == 0) {
        this->EMHandleDoor.Disable();
        this->EMHandleChamber.Disable();
        tempDoor = -100;
        tempChamber = -100;
      } else {
        this->EMHandleChamber.Enable();
        this->EMHandleDoor.Enable();
      }
  
      // Setup fans
      this->pinAssignment_Fan = fanPin;
      this->fanMode = fanMode;
      pinMode(this->pinAssignment_Fan, OUTPUT);
      if (this->fanMode == 4) {
        digitalWrite(this->pinAssignment_Fan, HIGH);       // Turn on the Fan  
      } else {
        digitalWrite(this->pinAssignment_Fan, LOW);        // Turn off the Fan
      }
    }
  
    void SetupNewHeating(int doorPin, int chamberPin, byte doorSensorPin, byte chamberSensorPin, byte auxSensorPin, int heatMode, int fanPin, int fanMode, float tempSetPoint) {
      #ifdef DEBUG_TEMP
        Serial.println(F("Heat::Setup"));
        Serial.println(doorPin);
        Serial.println(chamberPin);
        Serial.println(doorSensorPin);
        Serial.println(chamberSensorPin);
        Serial.println(auxSensorPin);
        Serial.println(heatMode);
        Serial.println(fanPin);
        Serial.println(fanMode);
      #endif

      // OldSchool hardware
      this->oneTempSensorInterface = false;
      
      // Setup EMs
      this->EMHandleChamber.SetupEM(char('C'), true, tempSetPoint, 0, chamberPin);
      this->EMHandleChamber.SetupEM_Timing(false, TEMP_ALARM_ON_PERIOD, 90.0, true, false, TEMPERATURE_STEP_LEN, false, 0.0);
      this->EMHandleChamber.setupEM_Alarms(true, TEMP_ALARM_THRESH, true, TEMP_ALARM_ON_PERIOD);
      this->EMHandleDoor.SetupEM(char('D'), true, tempSetPoint, 0, doorPin);
      this->EMHandleDoor.SetupEM_Timing(false, TEMP_ALARM_ON_PERIOD, 90.0, true, false, TEMPERATURE_STEP_LEN, false, 0.0);
      this->EMHandleDoor.setupEM_Alarms(true, TEMP_ALARM_THRESH, true, RESET_AFTER_DELTA);  // We have a really long alarm period for the door as we aren't as concerned if it never reaches its destination temperature
      
      // MakeSafe
      this->MakeSafeState();
      
      // Setup Temperature sensors
      if (auxSensorPin != 0) {
        this->oneWireA = new OneWire(auxSensorPin);                  // Setup a oneWire instance to communicate with aux oneWire devices
        this->tempSensorsA = new DallasTemperature(this->oneWireA);  // Pass our oneWireA reference to Dallas Temperature.
        this->tempSensorAux = true;
      } else {
        this->tempSensorAux = false;
      }

      if (chamberSensorPin != 0) {
        this->oneWireC = new OneWire(chamberSensorPin);              // Setup a oneWire instance to communicate with chamber oneWire devices
        this->tempSensorsC = new DallasTemperature(this->oneWireC);  // Pass our oneWireC reference to Dallas Temperature.
        this->tempSensorChamber = true;
      } else {
        this->tempSensorChamber = false;
      }

      if (doorSensorPin != 0) {
        this->oneWireD = new OneWire(doorSensorPin);                 // Setup a oneWire instance to communicate with door oneWire devices
        this->tempSensorsD = new DallasTemperature(this->oneWireD);  // Pass our oneWireD reference to Dallas Temperature.
        this->tempSensorDoor = true;
      } else {
        this->tempSensorDoor = false;
      }

      // Starting the temperature sensors
      this->tempSensorsA->begin();
      this->tempSensorsC->begin();
      this->tempSensorsD->begin();
    
      if (heatMode == 0) {
        this->EMHandleDoor.Disable();
        this->EMHandleChamber.Disable();
        tempDoor = -100;
        tempChamber = -100;
      } else {
        this->EMHandleChamber.Enable();
        this->EMHandleDoor.Enable();
      }
  
      // Setup fans
      this->pinAssignment_Fan = fanPin;
      this->fanMode = fanMode;
      pinMode(this->pinAssignment_Fan, OUTPUT);
      if (this->fanMode == 4) {
        digitalWrite(this->pinAssignment_Fan, HIGH);       // Turn on the Fan  
      } else {
        digitalWrite(this->pinAssignment_Fan, LOW);        // Turn off the Fan
      }
    }
  
    void SetSetPoint(float tempSetPoint) {
      this->EMHandleDoor.UpdateDesiredLevel(tempSetPoint);
      this->EMHandleChamber.UpdateDesiredLevel(tempSetPoint);
    }

    void UpdateHeatMode(int mode) {
      if (mode == 0) {
        this->EMHandleDoor.Disable();
        this->EMHandleChamber.Disable();
        tempDoor = -100;
        tempChamber = -100;
      } else {
        this->EMHandleChamber.Enable();
        this->EMHandleDoor.Enable();
      }
    }

    void UpdateFanMode(int mode) {
      this->fanMode = mode;
      if (this->fanMode == 4) {
        digitalWrite(this->pinAssignment_Fan, HIGH);       // Turn on the Fan  
      } else {
        digitalWrite(this->pinAssignment_Fan, LOW);        // Turn off the Fan
      }
    }
  
    void MakeSafeState() {
      #ifdef DEBUG_TEMP
        Serial.println(F("Heat::SafeState"));
      #endif
      this->EMHandleDoor.Disable();
      this->EMHandleChamber.Disable();
      digitalWrite(this->pinAssignment_Fan, LOW);        // Turn off the Fan
    }

    void ResumeState(int heatMode) {
      #ifdef DEBUG_TEMP
        Serial.println(F("Heat::ResumeState"));
      #endif
      if (heatMode != 0) {
        this->EMHandleChamber.Enable();
        this->EMHandleDoor.Enable();
      }
      if (this->fanMode == 4) {
        digitalWrite(this->pinAssignment_Fan, HIGH);       // Turn on the Fan  
      } 
    }

    void DoQuickTick() {
      this->EMHandleChamber.DoQuickTick();
      this->EMHandleDoor.DoQuickTick();
    }
    
    void DoTick() {
      if (this->oneTempSensorInterface) {
        this->GetTemperatureReadings();
      } else {
        this->GetNewTemperatureReadings();
      }
      this->EMHandleChamber.DoUpdateTick(this->tempChamber);
      this->EMHandleDoor.DoUpdateTick(this->tempDoor);
    }

    float getOtherTemperature() {
      return tempOther;
    }
    
    float getDoorTemperature() {
      return tempDoor;
    }

    boolean isDoorOn() {
      return this->EMHandleDoor.isActive();
    }

    boolean isDoorStepping() {
      return this->EMHandleDoor.isStepping();
    }

    float getChamberTemperature() {
      return tempChamber;
    }

    boolean isChamberOn() {
      return this->EMHandleChamber.isActive();
    }

    boolean isChamberStepping() {
      return this->EMHandleChamber.isStepping();
    }

    boolean isAlarmed() {
      if (this->EMHandleDoor.isAlarm_Overshoot() || this->EMHandleChamber.isAlarm_Overshoot() || this->EMHandleChamber.isAlarm_Undershoot()) {
        return true;
      } else {
        return false;
      }
    }

    void ResetAlarms() {
      // Deprecated
    }
};
