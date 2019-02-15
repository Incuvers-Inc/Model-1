#define TEMPERATURE_STEP_LEN 1200
#define TEMPERATURE_JUMP_LEN 60000
#define TEMPERATURE_JUMP_WT 60000
#define TEMP_ALARM_THRESH 114.0
#define TEMP_ALARM_ON_PERIOD 7200000

class IncuversHeatingSystem {
  private:
    int pinAssignment_Fan;
    int pinAssignment_OneWire;
    int fanMode;
    
    float tempDoor;
    float tempChamber;
    float tempOther;

    bool otherTempSensorPresent;
    
    byte sensorAddrDoorTemp[8];
    byte sensorAddrChamberTemp[8];
    byte sensorAddrOtherTemp[8];
    
    IncuversEM EMHandleDoor;
    IncuversEM EMHandleChamber;
    
    OneWire* oneWire;
    DallasTemperature* tempSensors;

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
      
      while(oneWire->search(addr)) {
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
        this->tempSensors->requestTemperatures();
        // Record the values
        tD = this->tempSensors->getTempC(this->sensorAddrDoorTemp);
        tC = this->tempSensors->getTempC(this->sensorAddrChamberTemp);
        if (this->otherTempSensorPresent) {
          tO = this->tempSensors->getTempC(this->sensorAddrOtherTemp);
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
  
    
  public:
    void SetupHeating(int doorPin, int chamberPin, int oneWirePin, byte doorSensorID[8], byte chamberSensorID[8], int heatMode, int fanPin, int fanMode, float tempSetPoint) {
      #ifdef DEBUG_TEMP
        Serial.println(F("Heat::Setup"));
        Serial.println(doorPin);
        Serial.println(chamberPin);
        Serial.println(oneWirePin);
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
      this->pinAssignment_OneWire = oneWirePin;
      this->oneWire = new OneWire(PINASSIGN_ONEWIRE_BUS);       // Setup a oneWire instance to communicate with ANY OneWire devices
      this->tempSensors = new DallasTemperature(this->oneWire);          // Pass our oneWire reference to Dallas Temperature.

      this->CheckForOtherTempSonsors();
      tempOther = -100;
      
      // Starting the temperature sensors
      this->tempSensors->begin();
    
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
    }

    void DoQuickTick() {
      this->EMHandleChamber.DoQuickTick();
      // Only doing Chamber as we are only Jolt-Ticking the door.
      this->EMHandleDoor.DoQuickTick();
    }
    
    void DoTick() {
      this->GetTemperatureReadings();
      this->EMHandleChamber.DoUpdateTick(this->tempChamber);
      this->EMHandleDoor.DoUpdateTick(this->tempDoor);
      //if (!this->EMHandleChamber.isActive()) {
        //this->EMHandleDoor.DoJoltTick(this->tempDoor);
      //}
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
      if (this->EMHandleDoor.isAlarm_Overshoot() || this->EMHandleDoor.isAlarm_Undershoot() || this->EMHandleChamber.isAlarm_Overshoot() || this->EMHandleChamber.isAlarm_Undershoot()) {
        return true;
      } else {
        return false;
      }
    }

    void ResetAlarms() {
      // Deprecated
    }
};
