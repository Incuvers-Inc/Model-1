class IncuversEM {
  private:
    boolean additiveElement;
    float desiredLevel;
    int outputPin
    
    boolean activeManagement;

    float mostRecentLevel;
    boolean inWork;
    boolean inStep;
    long startedWorkAt;
    long scheduledWorkEnd;

    long jumpDelta;
    boolean useStepping;
    float steppingLevel;
    long steppingDelta
    boolean useBleeding;
    long bleedDelta;
    boolean alarmOnOvershoot;
    float overshootAlarmLevel;
    boolean alarmOnUndershoot;
    long undershootAlarmDelta;


  void checkMaintenanceAdditive() {
    if (this->inWork) {
      // depending on the loop size, it may have been a significant fraction of a second since the last maintenanceTick, so lets do an extra one here, just in case we reached a level.
      DoMaintenanceTick();
    } else if (!this->inWork && this->mostRecentLevel < this->desiredLevel && this->MostRecentLevel > -999) {
      // We aren't doing any work, we have a valid result, and are below desired level
      if (!this->useBleeding || this->scheduledWorkEnd + this->bleedDelta < millis()) {
        // We are not using bleeding delays or it has expired
        if (this->useStepping && this->mostRecentLevel > this->steppingLevel && this->steppingDelta <= 750) {
          // We are in the stepping range and intend to step for shorter than a cycle
          digitalWrite(this->outputPin, HIGH);
          delay(this->steppingDelta);
          digitalWrite(this->outputPin, LOW);
          this->scheduledWorkEnd = millis();
        } else if (this->useStepping && this->mostRecentLevel > this->steppingLevel) {
          digitalWrite(this->outputPin, HIGH);
          this->startedWorkAt = millis();
          this->scheduledWorkEnd = this->startedWorkAt + this->steppingDelta;
          this->inWork = true;
          this->inStep = true;
        } else {
          digitalWrite(this->outputPin, HIGH);
          this->startedWorkAt = millis();
          this->scheduledWorkEnd = this->startedWorkAt + this->jumpDelta;
          this->inWork = true;
          this->inStep = false;
        }
      }
    } elseif (this->mostRecentLevel >= this->desiredLevel
  }

  
  void longthing() {
    #ifdef DEBUG_CO2 
        Serial.print(F("CO2Maintenance()"));
      #endif
      if (level < setPoint && level >= 0) {
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
                alarmUnder = true;
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
          alarmOver = true;
          #ifdef DEBUG_CO2 
            Serial.println(F("\tO2 over-saturation alarm"));
          #endif
        }
      }
  }


  public:
    void SetupEM(boolean additive, float level, int pin, long jmpDlt, boolean stepping, float stepLvl, long stepDlt, boolean bleed, long bleedDlt, boolean osAlrm, float osLvl, boolean usAlrm, long usDlt) {
      this->additiveElement = additive;
      this->desiredLevel = level;
      this->outputPin = pin;
      this->jumpDelta = jmpDlt;
      this->useStepping = stepping;
      this->steppingLevel = stepLvl;
      this->steppingDelta = stepDlt;
      this->useBleeding = bleed;
      this->bleedDelta = bleedDlt;
      this->alarmOnOvershoot = osAlrm;
      this->overshootAlarmLevel = osLvl;
      this->alarmOnUndershoot = usAlrm;
      this->undershootAlarmDelta = usDlt;

      this->activeManagement = false;
      this->mostRecentLevel = -999;
      this->inWork = false;
      this->inStep = false;
    }

    void DoUpdateTick(float newLevel) {
      this->mostRecentLevel = newLevel;
      
    }

    void DoMaintenanceTick() {
      if (this->activeManagement && this->inWork) {
        if (this->scheduledWorkEnd <= millis()) {
          digitalWrite(this->outputPin, LOW);
          #ifdef DEBUG_EM 
            Serial.print(F("Shut "));
            Serial.print(this->outputPin);
            Serial.print(F(" "));
            Serial.print((this->scheduledWorkEnd-millis()));
            Serial.println(F("ms late"));
          #endif
          this->inWork = false;
        } else if (this->inWork && this->most) {
          
        }
      }
    }



};
