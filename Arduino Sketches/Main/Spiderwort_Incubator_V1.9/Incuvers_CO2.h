#define CO2_STEP_THRESH 0.7
#define CO2_MULTIPLIER 10.0
#define CO2_DELTA_JUMP 3000
#define CO2_DELTA_STEPPING 250
#define CO2_BLEEDTIME_JUMP 5000
#define CO2_BLEEDTIME_STEPPING 5000
#define ALARM_THRESH 1.10
#define ALARM_CO2_OPEN_PERIOD 600000
#define ALARM_CO2_READING_PERIOD 1000

class IncuversCO2System {
  private:
    int pinAssignment_Valve;
    int mode;
    
    long lastCO2Check;
    long tickTime;
    long actionpoint;
    long setPointTime;
    long startCO2At;
    long shutCO2At;

    boolean enabled;
    boolean on;
    boolean stepping;
    boolean started;
    
    float level;
    float setPoint;
    
    IncuversSerialSensor* iSS;

    void CheckJumpStatus() {
      #ifdef DEBUG_CO2
        Serial.println(F("CO2::CheckJump"));
      #endif

      if (this->on) {
        if (this->tickTime >= this->shutCO2At) {
          digitalWrite(pinAssignment_Valve, LOW);
          #ifdef DEBUG_CO2 
            Serial.print(F("CO2 shut "));
            Serial.print((this->tickTime-this->shutCO2At));
            Serial.println(F("ms late"));
          #endif
          this->on = false;
          this->actionpoint = this->tickTime;
        }
      }
    }

    void GetCO2Reading_Cozir() {
      #ifdef DEBUG_CO2 
          Serial.println(F("GetCO2Reading_Cozir"));
      #endif
    
      String cozirString = "";
      int reading;
    
      for (int i = 0; i < 2; i++)
      {
        cozirString = this->iSS->GetSerialSensorReading(10, 17);
        // Data in the stream looks like "Z 00400 z 00360"
        // The first number is the filtered value and the number after 
        // the 'z' is the raw value. We want the filtered value
    
        reading = GetIntegerSensorReading('Z', cozirString, -1);
    
        if (reading > 0 && reading < 30) {
          level = (float)((CO2_MULTIPLIER * reading)/10000);  
          #ifdef  DEBUG_CO2
            Serial.print("  CO2 level: ");
            Serial.println(level);
          #endif
          i = 2; // escape the loop.
        } else {
          #ifdef DEBUG_CO2
            Serial.print(F("\tCO2 sensor returned invalid read, attempt"));
            Serial.println(i);
          #endif 
        }
      }
    }
    
    void CheckCO2Maintenance() {
      #ifdef DEBUG_CO2 
        Serial.print(F("CO2Maintenance()"));
      #endif
      if (level < setPoint ) {
        if (level > (setPoint * CO2_STEP_THRESH)) {
          if (this->tickTime > actionpoint + CO2_BLEEDTIME_STEPPING) {
            // In stepping mode and not worried about bleed delay.
            digitalWrite(pinAssignment_Valve, HIGH);
            delay(CO2_DELTA_STEPPING);
            digitalWrite(pinAssignment_Valve, LOW);
            this->actionpoint = this->tickTime;
            #ifdef DEBUG_CO2 
              Serial.println(F("\tCO2 step mode"));
            #endif
          } // there is no else, we need to wait for the bleedtime to expire.
        } else {
          // below the setpoint and the stepping threshold, 
          if (!this->on && this->tickTime > (this->actionpoint + CO2_BLEEDTIME_JUMP)) {
            if (this->started == false) {
              this->started = true;
              this->startCO2At = this->tickTime;
            } else {
              if (this->startCO2At + ALARM_CO2_OPEN_PERIOD < this->tickTime) {
                //statusHolder.AlarmCO2UnderSaturation = true;
                #ifdef DEBUG_CO2 
                  Serial.println(F("\tCO2 under-saturation alarm thrown"));
                #endif
              }
            }
            this->on = true;
            this->shutCO2At = (this->tickTime + CO2_DELTA_JUMP);
            digitalWrite(pinAssignment_Valve, HIGH);
            #ifdef DEBUG_CO2 
              Serial.print(F("\tCO2 opening from "));
              Serial.print(this->tickTime);
              Serial.print(F(" until "));
              Serial.println(this->shutCO2At);
            #endif
          } else {
           #ifdef DEBUG_CO2 
             Serial.println(F("\tCO2 under but bleed remaining"));
           #endif 
          }
        }
      } else {
        // CO2 level above setpoint.
        digitalWrite(pinAssignment_Valve, LOW); // just to make sure
        this->started = false;
        if (level > (setPoint * ALARM_THRESH)) {
          // Alarm
          //statusHolder.AlarmCO2OverSaturation = true;
          #ifdef DEBUG_CO2 
            Serial.println(F("\tO2 over-saturation alarm"));
          #endif
        }
      }
    }

  public:
    void SetupCO2(int rxPin, int txPin, int gasMode, int relayPin) {
      #ifdef DEBUG_CO2
        Serial.println(F("CO2::Setup"));
      #endif
      
      if (gasMode == 0) {
        #ifdef DEBUG_CO2
          Serial.println(F("Disabled"));
        #endif
        this->enabled = false;
        level = -100;
      } else {
        this->enabled = true;
        // Setup Serial Interface
        this->iSS = new IncuversSerialSensor();
        this->iSS->Initialize(rxPin, txPin, true); 
        
        //Setup the gas system
        this->pinAssignment_Valve = relayPin;
        pinMode(this->pinAssignment_Valve, OUTPUT);
        
        #ifdef DEBUG_CO2
          Serial.println(F("Enabled"));
          Serial.print(F("  Rx: "));
          Serial.println(rxPin);
          Serial.print(F("  Tx: "));
          Serial.println(txPin);
          Serial.print(F("  Relay: "));
          Serial.println(relayPin);
        #endif
  
        this->MakeSafeState();
      }
  
      this->mode = gasMode;
    }

    void SetSetPoint(float tempSetPoint) {
      this->setPoint = tempSetPoint;
      this->setPointTime = millis();
    }
    
    void MakeSafeState() {
      if (this->enabled) {
        digitalWrite(this->pinAssignment_Valve, LOW);   // Set LOW (solenoid closed off)
        this->on = false;
        this->stepping = false;
        this->started = false;
      }
    }

    void DoTick() {
      if (this->enabled) {
        this->tickTime = millis();
      
        if (!this->stepping) {
          this->CheckJumpStatus();  
        }

        this->GetCO2Reading_Cozir();

        this->CheckCO2Maintenance();
      }
    }

    float getCO2Level() {
      return level;
    }

    boolean isCO2Open() {
      return on;
    }

    boolean isCO2Stepping() {
      return stepping;
    }

    void UpdateMode(int mode) {
      this->enabled = false;
    }

};
