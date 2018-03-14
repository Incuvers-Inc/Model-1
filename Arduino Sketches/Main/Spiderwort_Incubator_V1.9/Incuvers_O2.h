#define OO_STEP_THRESH 1.01
#define N_DELTA_JUMP 10000
#define N_DELTA_STEPPING 250
#define N_BLEEDTIME_JUMP 2500
#define N_BLEEDTIME_STEPPING 5000
#define ALARM_THRESH 1.10
#define ALARM_O2_READING_PERIOD 1000

class IncuversO2System {
  private:
    int pinAssignment_Valve;
    int mode;
    
    long lastO2Check;
    long tickTime;
    long setPointTime;
    long startO2At;
    long shutO2At;
    long actionpoint;

    boolean enabled;
    boolean on;
    boolean stepping;
    boolean started;
    
    float level;
    float setPoint;
    
    IncuversSerialSensor* iSS;

    void CheckJumpStatus() {
      #ifdef DEBUG_O2
        Serial.println(F("O2::CheckJump"));
      #endif

      if (this->on) {
        if (this->tickTime >= this->shutO2At) {
          digitalWrite(pinAssignment_Valve, LOW);
          #ifdef DEBUG_O2
            Serial.print(F("O2 shut "));
            Serial.print((this->tickTime-this->shutO2At));
            Serial.println(F("ms late"));
          #endif
          this->on = false;
          this->actionpoint = this->tickTime;
        }
      }
    }

    void GetO2Reading_Luminox() {
      #ifdef DEBUG_O2 
          Serial.println(F("O2Reading_Luminox"));
      #endif
    
      String luminoxString = "";
      float reading;
      
      for (int i = 0; i < 2; i++)
      {
        luminoxString = this->iSS->GetSerialSensorReading(38, 44);
        // Data in the stream looks like "O 0211.3 T +29.3 P 1011 % 020.90 e 0000"
        //                                O=ppO2 in mbar   P pressure in mbar
        //                                         T=temp in deg C         e=sensor errors
        //                                                        %=O2 concentration in %
        
        reading = GetDecimalSensorReading('%', luminoxString, -1);
    
        if (reading > 0 && reading < 30) {
          level = reading;  
          #ifdef DEBUG_O2
            Serial.print("  O2 level is detected to be: ");
            Serial.println(level);
          #endif
          i = 5; // escape the loop.
        } else {
          #ifdef DEBUG_O2
            Serial.print(F("\tO2 sensor returned invalid reading attempt "));
            Serial.println(i);
          #endif 
        }
      }
    }
    
    void CheckO2Maintenance() {
      if (level > setPoint ) {
        if (level < (setPoint * OO_STEP_THRESH)) {
          if (tickTime > actionpoint + N_BLEEDTIME_STEPPING) {
            // In stepping mode and not worried about bleed delay.
            digitalWrite(pinAssignment_Valve, HIGH);
            delay(N_DELTA_STEPPING);
            digitalWrite(pinAssignment_Valve, LOW);
            actionpoint = tickTime;
            #ifdef DEBUG_O2
              Serial.println(F("\tO2 step mode"));
            #endif
          } // there is no else, we need to wait for the bleedtime to expire.
        } else {
          // below the setpoint and the stepping threshold, 
          if (!on && tickTime > (actionpoint + N_BLEEDTIME_JUMP)) {
            if (started == false) {
              started = true;
              startO2At = tickTime;
            } else {
              if (startO2At + ALARM_CO2_OPEN_PERIOD < tickTime) {
                //statusHolder.AlarmO2UnderSaturation = true;
                #ifdef DEBUG_O2
                  Serial.println(F("\tO2 under-saturation alarm"));
                #endif
              }
            }
            on = true;
            shutO2At = (tickTime + N_DELTA_JUMP);
            digitalWrite(pinAssignment_Valve, HIGH);
            #ifdef DEBUG_O2
             Serial.print(F("\tN jump from "));
              Serial.print(tickTime);
              Serial.print(F(" to "));
              Serial.println(shutO2At);
            #endif
          } else {
           #ifdef DEBUG_O2
             Serial.println(F("\tO2 over-saturated but bleeding"));
           #endif
          }
        }
      } else {
        // O2 level below setpoint.
        digitalWrite(pinAssignment_Valve, LOW); // just to make sure
        started = false;
        if (level > (setPoint * (1.0 - ALARM_THRESH))) {
          // Alarm
          //statusHolder.AlarmO2OverSaturation = true;
          #ifdef DEBUG_O2
            Serial.println(F("\tO2 over-saturation alarm"));
          #endif
        }
      }
    }

  public:
    void SetupO2(int rxPin, int txPin, int gasMode, int relayPin) {
      #ifdef DEBUG_O2
        Serial.println(F("O2::Setup"));
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
        this->MakeSafeState();
        
        #ifdef DEBUG_O2
          Serial.println(F("Enabled."));
          Serial.print(F("\tRx: "));
          Serial.println(rxPin);
          Serial.print(F("\tTx: "));
          Serial.println(txPin);
          Serial.print(F("\tRelay: "));
          Serial.println(relayPin);
        #endif
      }
  
      this->mode = gasMode;
    }

    void SetSetPoint(float tempSetPoint) {
      this->setPoint = tempSetPoint;
      this->setPointTime = millis();
    }
    
    void MakeSafeState() {
      digitalWrite(this->pinAssignment_Valve, LOW);   // Set LOW (solenoid closed off)
      this->on = false;
      this->stepping = false;
      this->started = false;
    }

    void DoTick() {
      if (this->enabled) {
        this->tickTime = millis();
      
        if (!this->stepping) {
          this->CheckJumpStatus();  
        }

        this->GetO2Reading_Luminox();
      }
    }

    float getO2Level() {
      return level;
    }

    boolean isNOpen() {
      return on;
    }

    boolean isNStepping() {
      return stepping;
    }

    void UpdateMode(int mode) {
      this->enabled = false;
    }

};
