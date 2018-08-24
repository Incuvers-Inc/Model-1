#define EM_BLOCKTHREADBELOWMS 750

class IncuversEM {
  private:
    char ident;                     // Character to identify EM Instance in Debugging output.
    
    boolean additiveElement;        // Are we trying to go up to the desired level (versus down to it)
    float desiredLevel;             // What level we are trying to reach
    float defaultLevel;             // What is the default level we are trying to get from (used only when descending)
    int outputPin                   // Where to send a signal to help get to the desired level
    
    boolean activeManagement;       // Is this Environmental Manager active?

    float mostRecentLevel;          // What level we are currently at
    float percentageToDesired;      // How close we are to the target value
    boolean inWork;                 // Currently working to get to the value (not reset until goal is reached.)
    boolean activeWork;             // Currently doing a unit of work.
    boolean inStep;                 // Currently in stepping mode
    long startedWorkAt;             // When the work started (not reset until the goal is reached)
    long scheduledWorkEnd;          // When the current work unit is scheduled to end.

    boolean useJumpLength           // Use a specific jump length instead of constant use
    long jumpDelta;                 // What the jump time is in ms (if set to jump)
    float jumpPercentageLimit;      // Up to what percentage of the desired level do we jump (90% seems good)
    boolean useStepping;            // When close to the desired value, do we switch to short burts to avoid an overshoot?
    boolean flatStepping;           // Always use the same stepping length (versus exponential)
    long steppingDelta              // What is the base length of time to use when stepping (used for flat or base number for exponential)
    boolean useBleeding;            // After completing a bit of work, wait before starting another bit of work?
    long bleedDelta;                // How long to wait before starting another bit of work.
    
    // alarm items
    boolean alarmOnOvershoot;       // Raise alarm if we go past our desired level
    float overshootAlarmLevel;      // At what level does the alarm sound?
    boolean alarmOnUndershoot;      // Raise alarm if we don't reach our desired level in a timely time
    long undershootAlarmDelta;      // At what time does this alarm sound?
    boolean alarmSupressor;         // Flag to supress alarm if a desiredLevel has been changed.
    

  void DoStep(long len) {
    long now = millis();
    
    if (len <= <= EM_BLOCKTHREADBELOWMS) {
      // We are in the stepping range and intend to step for shorter than a cycle
      digitalWrite(this->outputPin, HIGH);
      delay(len);
      digitalWrite(this->outputPin, LOW);
      this->scheduledWorkEnd = now;
    } else {
      digitalWrite(this->outputPin, HIGH);
      if (!this->inWork) {
        this->startedWorkAt = millis();
        this->scheduledWorkEnd = this->startedWorkAt + len;
        this->inWork = true;
      } else {
        this->scheduledWorkEnd = now + len;
      }
      this->activeWork = true;
      this->inStep = true;
    }
  }

  long CalculateExponentialStepLength() {
    float expMult exp((100.0 - this->percentageToDesired) * 0.1)
    float fullLen = expMult * expMult * this->steppingDelta;
    long len = (long)fullLen;

    #ifdef DEBUG_EM 
      Serial.print(this->ident);
      Serial.print(F(" :: CalcExpStpLen "));
      Serial.print(len);
      Serial.print(F(" (@"));
      Serial.print(this->percentageToDesired);
      Serial.print(F(" w/ "));
      Serial.print(this->steppingDelta);
      Serial.println(F("ms)"));
    #endif
    
    return len;
  }

  void CheckMaintenance() {
    long nowStamp = millis();
    
    if (this->percentageToDesired >= 100.0 && this->inWork) {
      digitalWrite(this->outputPin, LOW);
      this->inWork = false;
      this->activeWork = false;
      this->inStep = false;
    } else if (this->percentageToDesired < 100.0) {
      // we aren't at the desired level
      if (this->alarmSupressor) {
        // We had been supressing the alarms after a change, we don't need to do that anymore
        this->alarmSupressor = false;
      }

      if ((this->activeWork && this->percentageToDesired > this->jumpPercentageLimit && this->useStepping && !this->inStep) || (!this->activeWork && (!this->useBleeding || this->scheduledWorkEnd + this->bleedDelta < nowStamp))) {
        // We either aren't already doing anything and we aren't waiting on a bleed or we are not in step mode but in step territory so should start a stepping cycle.
        if (this->useStepping && this->percentageToDesired > this->jumpPercentageLimit) {
          // Stepping mode
          if (this->flatStepping) {
            this->DoStep(this->steppingDelta);
          } else {
            this->DoStep(this->CalculateExponentialStepLength);
          }
        }
        
      }
    }

    
    if (this->inWork) {
      // depending on the loop size, it may have been a significant fraction of a second since the last maintenanceTick, so lets do an extra one here, just in case we reached a level.
      DoMaintenanceTick();
    } else if (!this->inWork && this->mostRecentLevel < this->desiredLevel && this->MostRecentLevel > -999) {
      // We aren't doing any work, we have a valid result, and are below desired level
      if (!this->useBleeding || this->scheduledWorkEnd + this->bleedDelta < millis()) {
        // We are not using bleeding delays or it has expired
        if (this->useStepping && this->mostRecentLevel > this->steppingLevel && this->steppingDelta <= EM_BLOCKTHREADBELOWMS) {
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
    void SetupEM(char id, boolean additive, float level, float def, int pin) {
      this->ident = id;
      this->additiveElement = additive;
      this->desiredLevel = level;
      this->defaultLevel = def;
      this->outputPin = pin;
      
      this->activeManagement = false;
      this->mostRecentLevel = -100;
      this->inWork = false;
      this->inStep = false;
    }

    void SetupEM_Timing(boolean useStaticJump, long jmpDlt, float jmpPct, boolean useStp, boolean fltStp, long stpDlt, boolean useBld, long bldDlt) {
      this->useJumpLength = useStaticJump
      this->jumpDelta = jmpDlt;
      this->jumpPercentageLimit = jmpPct;
      this->useStepping = useStp;
      this->flatStepping = fltStp
      this->steppingDelta = stpDlt;
      this->useBleeding = useBld;
      this->bleedDelta = bldDlt;
    }

    void setupEM_Alarms(boolean osAlrm, float osLvl, boolean usAlrm, long usDlt) {
      this->alarmOnOvershoot = osAlrm;
      this->overshootAlarmLevel = osLvl;
      this->alarmOnUndershoot = usAlrm;
      this->undershootAlarmDelta = usDlt;

      this->alarmSupressor = false;
    }

    void Enable() {
      this->activeManagement = true;
    }

    void Disable() {
      this->activeManagement = false;
      
      digitalWrite(this->outputPin, LOW);
      this->inWork = false;
      this->inStep = false;
    }

    void UpdateDesiredLevel(float level) {
      if (this->additiveElement) {
        if (level < this->desiredLevel) { 
          // Keep the lab a quiet environment by ensuring the alarm won't sound due to this change.
          this->alarmSupressor = true;
        }
      } else {
        if (level > this->desiredLevel) { 
          // Keep the lab a quiet environment by ensuring the alarm won't sound due to this change.
          this->alarmSupressor = true;
        }
      }
      
      this->desiredLevel = level;
    }

    void DoUpdateTick(float newLevel) {
      this->mostRecentLevel = newLevel;

      if (this->activeManagement) {
        this->DoMaintenanceTick();
        
        if (this->additiveElement) {
          this->percentageToDesired = this->mostRecentLevel / this->desiredLevel;
          this->checkMaintenanceAdditive();
        } else {
          this->percentageToDesired = (this->defaultLevel - this->mostRecentLevel) / (this->defaultLevel - this->desiredLevel);
        }
      }
    }

    void DoQuickTick() {
      long nowTime = millis();
      
      if (this->activeManagement && this->inWork) {
        if (this->scheduledWorkEnd <= nowTime) {
          digitalWrite(this->outputPin, LOW);
          #ifdef DEBUG_EM 
            Serial.print(this->ident);
            Serial.print(F(" :: Shut "));
            Serial.print(this->outputPin);
            Serial.print(F(" "));
            Serial.print((this->scheduledWorkEnd - nowTime));
            Serial.println(F("ms late"));
          #endif
          this->inWork = false;
        } 
      }
    }



};
