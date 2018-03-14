#define TEMPERATURE_READING_DELTA 500
#define TEMPERATURE_STEP_THRESH 0.998
#define TEMPERATURE_STEP_LEN 2000
#define ALARM_THRESH 1.10
#define ALARM_TEMP_ON_PERIOD 1800000

class IncuversHeatingSystem {
  private:
    int pinAssignment_Door;
    int pinAssignment_Chamber;
    int pinAssignment_Fan;
    int pinAssignment_OneWire;
    int fanMode;
    
    long lastTempCheck;
    long tickTime;
    long setPointTime;
    long startDoorHeatAt;
    long startChamberHeatAt;
    long shutDoorHeatAt;
    long shutChamberHeatAt;

    boolean heatEnabled;
    boolean heatOn_Door;
    boolean stepping_Door;
    boolean heatOn_Chamber;
    boolean stepping_Chamber;

    float tempDoor;
    float tempChamber;
    float setPoint;
    
    byte sensorAddrDoorTemp[8];
    byte sensorAddrChamberTemp[8];
    
    OneWire* oneWire;
    DallasTemperature* tempSensors;
    
    void GetTemperatureReadings() {
      #ifdef DEBUG_TEMP
        Serial.println(F("Heat::GetTempRead"));
      #endif
      boolean updateCompleted = false;
      int i = 0;
      
      while (!updateCompleted) {
        // Request the temperatures
        this->tempSensors->requestTemperatures();
        // Record the values
        this->tempDoor = this->tempSensors->getTempC(this->sensorAddrDoorTemp);
        this->tempChamber = this->tempSensors->getTempC(this->sensorAddrChamberTemp);
        
        #ifdef DEBUG_TEMP
          Serial.print(F("Door: "));
          Serial.print(this->tempDoor);
          Serial.print(F("*C  Chamber: "));
          Serial.print(this->tempChamber);
          Serial.println("*C");
        #endif  
        if (this->tempDoor < 85.0 && this->tempChamber < 85.0 && this->tempDoor > -100 && this->tempChamber > -100) {
          // we have valid readings
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
  
    void CheckSteppingStatus() {
      #ifdef DEBUG_TEMP
        Serial.println(F("Heat::CheckStep"));
      #endif
      if (this->heatOn_Chamber && this->stepping_Chamber) {
        if (this->tickTime >= this->shutChamberHeatAt) {
          digitalWrite(this->pinAssignment_Chamber, LOW);
          #ifdef DEBUG_TEMP
            Serial.print(F("Shut chamber "));
            Serial.print((this->tickTime - this->shutChamberHeatAt));
            Serial.println(F("ms late"));
          #endif
          this->heatOn_Chamber = false;
          this->stepping_Chamber = false;
        }
      }
  
      if (this->heatOn_Door && this->stepping_Door) {
        if (this->tickTime >= this->shutDoorHeatAt) {
          digitalWrite(this->pinAssignment_Door, LOW);
          #ifdef DEBUG_TEMP
            Serial.print(F("Shut door "));
            Serial.print((this->tickTime - this->shutDoorHeatAt));
            Serial.println(F("ms late"));
          #endif
          this->heatOn_Door = false;
          this->stepping_Door = false;
        }
      }
    }

    void CheckHeatMaintenance() {
      // Chamber Temperature
      if (tempChamber < setPoint) {
        if (tempChamber > (setPoint * TEMPERATURE_STEP_THRESH)) {
          // stepping heat
          if (heatOn_Chamber) {
            // Already in stepping mode
          } else {
            // Stepping mode enabled!
            digitalWrite(PINASSIGN_HEATCHAMBER, HIGH);
            heatOn_Chamber = true;
            stepping_Chamber = true;
            shutChamberHeatAt = tickTime + TEMPERATURE_STEP_LEN;
            #ifdef DEBUG_TEMP
            Serial.println(F("Chamber step mode"));
            #endif
          }
        } else {
          // non-stepping heat
          if (heatOn_Chamber) {
            // already engaged, check for delta
            if (shutChamberHeatAt < tickTime) {
              //statusHolder.AlarmHeatChamberUnderTemp = true;
            }
          } else {
            digitalWrite(PINASSIGN_HEATCHAMBER, HIGH);
            heatOn_Chamber = true;
            stepping_Chamber = false;
            shutChamberHeatAt = tickTime + ALARM_TEMP_ON_PERIOD;
            #ifdef DEBUG_TEMP
            Serial.println(F("Chamber jump"));
            #endif
          }
        }
      } else {
        // Chamber temp. is over setpoint.
        if (heatOn_Chamber && !stepping_Chamber) {
          // turn off the heater
          digitalWrite(PINASSIGN_HEATCHAMBER, LOW);
          heatOn_Chamber = false;
          #ifdef DEBUG_TEMP
          Serial.println(F("Chamber shutdown"));
          #endif
        }
        if (tempChamber > (setPoint * ALARM_THRESH)) {
          // Alarm
          //statusHolder.AlarmHeatChamberOverTemp = true;
          #ifdef DEBUG_TEMP
          Serial.println(F("Chamber over-temp alarm"));
          #endif
        }
      }
  
      // Door Temperature
      if (tempDoor < setPoint) {
        if (tempDoor > (setPoint * TEMPERATURE_STEP_THRESH)) {
          // stepping heat
          if (heatOn_Door) {
            // Already in stepping mode
          } else {
            // Stepping mode enabled!
            digitalWrite(PINASSIGN_HEATDOOR, HIGH);
            heatOn_Door = true;
            stepping_Door = true;
            shutDoorHeatAt = tickTime + TEMPERATURE_STEP_LEN;
            #ifdef DEBUG_TEMP
            Serial.println(F("Door step mode"));
            #endif
          }
        } else {
          // non-stepping heat
          if (heatOn_Door) {
            // already engaged, check for delta
            if (shutDoorHeatAt < tickTime) {
              //statusHolder.AlarmHeatDoorUnderTemp = true;
            }
          } else {
            digitalWrite(PINASSIGN_HEATDOOR, HIGH);
            heatOn_Door = true;
            stepping_Door = false;
            shutDoorHeatAt = tickTime + ALARM_TEMP_ON_PERIOD;
            #ifdef DEBUG_TEMP
            Serial.println(F("Door jump mode"));
            #endif
          }
        }
      } else {
        // Chamber temp. is over setpoint.
        if (heatOn_Door && !stepping_Door) {
          // turn off the heater
          digitalWrite(PINASSIGN_HEATDOOR, LOW);
          heatOn_Door = false;
          #ifdef DEBUG_TEMP
          Serial.println(F("Door shutdown"));
          #endif
        }
        if (tempDoor > (setPoint * ALARM_THRESH)) {
          // Alarm
          //statusHolder.AlarmHeatDoorOverTemp = true;
          #ifdef DEBUG_TEMP
          Serial.println(F("Door over-temp alarm"));
          #endif
        }
      }
    }
 
  public:
    void SetupHeating(int doorPin, int chamberPin, int oneWirePin, byte doorSensorID[8], byte chamberSensorID[8], int heatMode, int fanPin, int fanMode) {
      #ifdef DEBUG_TEMP
        Serial.println(F("Heat::Setup"));
      #endif
      
      // Setup heaters 
      this->pinAssignment_Door = doorPin;
      this->pinAssignment_Chamber = chamberPin;
      pinMode(this->pinAssignment_Chamber, OUTPUT);  
      pinMode(this->pinAssignment_Door,OUTPUT);      
      this->MakeSafeState();
      
      // Setup Temperature sensors
      this->pinAssignment_OneWire = oneWirePin;
      this->oneWire = new OneWire(PINASSIGN_ONEWIRE_BUS);       // Setup a oneWire instance to communicate with ANY OneWire devices
      this->tempSensors = new DallasTemperature(this->oneWire);          // Pass our oneWire reference to Dallas Temperature.
  
      // Starting the temperature sensors
      this->tempSensors->begin();
  
      for (int i = 0; i < 8; i++) {
        this->sensorAddrDoorTemp[i] = doorSensorID[i];
        this->sensorAddrChamberTemp[i] = chamberSensorID[i];
      }
  
      if (heatMode == 0) {
        this->heatEnabled = false;
        tempDoor = -100;
        tempChamber = -100;
      } else {
        this->heatEnabled = true;
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
      this->setPoint = tempSetPoint;
      this->setPointTime = millis();
    }

    void UpdateHeatMode(int mode) {
      if (mode == 0) {
        heatEnabled = false;
        tempDoor = -100;
        tempChamber = -100;
      } else {
        this->heatEnabled = true;
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
      digitalWrite(this->pinAssignment_Door, LOW);     // Set LOW (heater off)
      digitalWrite(this->pinAssignment_Chamber, LOW);  // Set LOW (heater off)
      this->heatOn_Door = false;
      this->stepping_Door = false;
      this->heatOn_Chamber = false;
      this->stepping_Chamber = false;
    }
  
    void DoTick() {
      this->tickTime = millis();
      
      if (this->heatEnabled) {
        if (this->stepping_Door || this->stepping_Chamber) {
          this->CheckSteppingStatus();  
        }
      }
  
      if ((long)TEMPERATURE_READING_DELTA + this->lastTempCheck < this->tickTime) {
        this->GetTemperatureReadings();
      }
  
      CheckHeatMaintenance();
    }

    float getDoorTemperature() {
      return tempDoor;
    }

    boolean isDoorOn() {
      return heatOn_Door;
    }

    boolean isDoorStepping() {
      return stepping_Door;
    }

    float getChamberTemperature() {
      return tempChamber;
    }

    boolean isChamberOn() {
      return heatOn_Chamber;
    }

    boolean isChamberStepping() {
      return stepping_Chamber;
    }

};






