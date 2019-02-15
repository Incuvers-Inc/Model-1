#define EM_BLOCKTHREADBELOWMS 650
#define EM_MAXJUMPLEN 3600000

class IncuversEM {
  private:
    char ident;                     // Character to identify EM Instance in Debugging output.

    boolean additiveElement;        // Are we trying to go up to the desired level (versus down to it)
    float desiredLevel;             // What level we are trying to reach
    float defaultLevel;             // What is the default level we are trying to get from (used only when descending)
    int outputPin;                  // Where to send a signal to help get to the desired level

    boolean activeManagement;       // Is this Environmental Manager active?

    float mostRecentLevel;          // What level we are currently at
    float percentageToDesired;      // How close we are to the target value
    boolean inWork;                 // Currently working to get to the value (not reset until goal is reached.)
    boolean activeWork;             // Currently doing a unit of work.
    boolean inStep;                 // Currently in stepping mode
    long startedWorkAt;             // When the work started (not reset until the goal is reached)
    long scheduledWorkEnd;          // When the current work unit is scheduled to end.

    boolean useJumpLength;          // Use a specific jump length instead of constant use
    long jumpDelta;                 // What the jump time is in ms (if set to jump)
    float jumpPercentageLimit;      // Up to what percentage of the desired level do we jump (90% seems good)
    boolean useStepping;            // When close to the desired value, do we switch to short burts to avoid an overshoot?
    boolean flatStepping;           // Always use the same stepping length (versus exponential)
    long steppingDelta;             // What is the base length of time to use when stepping (used for flat or base number for exponential)
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

    if (len <= EM_BLOCKTHREADBELOWMS) {
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
    float expMult = exp((100.0 - this->percentageToDesired) * 0.1);
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
    #ifdef DEBUG_EM
      Serial.print(F("CheckMaintenance (@"));
      Serial.print(nowStamp);
      Serial.print(F("ms @@ "));
      Serial.print(this->percentageToDesired);
      Serial.println(F("%)"));
    #endif

    if (this->percentageToDesired >= 100.1 && this->activeWork) {
      #ifdef DEBUG_EM
        Serial.print(F("  "));
        Serial.print((this->ident));
        Serial.println(F(": We are over 100% to our target, shutdown time"));

      #endif
      digitalWrite(this->outputPin, LOW);
      this->inWork = false;
      this->activeWork = false;
      this->inStep = false;
    } else if (this->percentageToDesired < 100.1) {
      // we aren't at the desired level, or are just over so we give a little nudge to keep things where they should be.
      if (this->alarmSupressor) {
        // We had been supressing the alarms after a change, we don't need to do that anymore
        this->alarmSupressor = false;
      }

      if ((this->activeWork && this->percentageToDesired > this->jumpPercentageLimit && this->useStepping && !this->inStep) || (!this->activeWork && (!this->useBleeding || this->scheduledWorkEnd + this->bleedDelta < nowStamp))) {
        // We either aren't already doing anything and we aren't waiting on a bleed or we are not in step mode but in step territory so should start a stepping cycle.
        if (this->useStepping && this->percentageToDesired > this->jumpPercentageLimit) {
          // Stepping mode
          #ifdef DEBUG_EM
            Serial.print(F("  "));
            Serial.print((this->ident));
            Serial.println(F(": We are not at our target, but close, stepping!"));
          #endif
          if (this->flatStepping) {
            this->DoStep(this->steppingDelta);
          } else {
            this->DoStep(this->CalculateExponentialStepLength());
          }
          this->inWork = true;
          this->activeWork = true;
          this->inStep = true;
          this->startedWorkAt = millis();
        } else {
          digitalWrite(this->outputPin, HIGH);
          if (this->useJumpLength) {
            #ifdef DEBUG_EM
              Serial.print(F("  "));
              Serial.print((this->ident));
              Serial.println(F(": We are starting a new jump"));
            #endif
            this->scheduledWorkEnd = this->startedWorkAt + this->jumpDelta;
          } else {
            this->scheduledWorkEnd = this->startedWorkAt + EM_MAXJUMPLEN;
          }
          this->inWork = true;
          this->activeWork = true;
          this->inStep = false;
          this->startedWorkAt = millis();
        }
      } else {
        if (this->activeWork && !this->inStep && !this->useBleeding) {
          // we are jumping, re-enable, just in case
          #ifdef DEBUG_EM
            Serial.print(F("  "));
            Serial.print((this->ident));
            Serial.println(F(": I'm jumping, but making sure"));
          #endif
          digitalWrite(this->outputPin, HIGH);
        } else {
          #ifdef DEBUG_EM
            Serial.print(F("  "));
            Serial.print((this->ident));
            Serial.println(F(": I'm bleeding or busy jumping"));
          #endif
        }
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

      pinMode(this->outputPin, OUTPUT);
      digitalWrite(this->outputPin, LOW);

      this->activeManagement = false;
      this->mostRecentLevel = -100;
      this->inWork = false;
      this->inStep = false;
    }

    void SetupEM_Timing(boolean useStaticJump, long jmpDlt, float jmpPct, boolean useStp, boolean fltStp, long stpDlt, boolean useBld, long bldDlt) {
      this->useJumpLength = useStaticJump;
      this->jumpDelta = jmpDlt;
      this->jumpPercentageLimit = jmpPct;
      this->useStepping = useStp;
      this->flatStepping = fltStp;
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
      #ifdef DEBUG_EM
        Serial.println(F("Enablement"));
      #endif

      this->activeManagement = true;
    }

    void Disable() {
      #ifdef DEBUG_EM
        Serial.println(F("Disablement"));
      #endif
      this->activeManagement = false;

      digitalWrite(this->outputPin, LOW);
      this->inWork = false;
      this->inStep = false;
    }

    void UpdateDesiredLevel(float level) {
      #ifdef DEBUG_EM
        Serial.println(F("UpdateLevel"));
      #endif
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
      #ifdef DEBUG_EM
        Serial.print(F("UpdateTick @ "));
        Serial.print(newLevel);
        Serial.println(F("!"));
      #endif
      this->mostRecentLevel = newLevel;

      if (this->activeManagement) {
        this->DoQuickTick();

        if (this->additiveElement) {
          this->percentageToDesired = this->mostRecentLevel / this->desiredLevel * 100;
        } else {
          this->percentageToDesired = (this->defaultLevel - this->mostRecentLevel) / (this->defaultLevel - this->desiredLevel) * 100;
        }

        this->CheckMaintenance();
      }
    }

    void DoQuickTick() {
      long nowTime = millis();

      if (this->activeManagement && this->activeWork) {
        if (this->scheduledWorkEnd <= nowTime) {
          digitalWrite(this->outputPin, LOW);
          #ifdef DEBUG_EM
            Serial.print(this->ident);
            Serial.print(F(" :: Shut "));
            Serial.print(this->outputPin);
            Serial.print(F(" "));
            Serial.print((nowTime - this->scheduledWorkEnd));
            Serial.println(F("ms late"));
          #endif
          this->activeWork = false;
        }
      }
    }

    void DoJoltTick(float newLevel) {
      /*
       * This is a new function which is designed to allow the door to be Jolted for 0.5 secs between work on the chamber for power management reasons.
       */
      this->mostRecentLevel = newLevel;
      if (this->activeManagement && !this->inWork) {
        if (this->additiveElement) {
          this->percentageToDesired = this->mostRecentLevel / this->desiredLevel;
        } else {
          this->percentageToDesired = (this->defaultLevel - this->mostRecentLevel) / (this->defaultLevel - this->desiredLevel);
        }

        if (this->percentageToDesired < 100.0) {
          digitalWrite(this->outputPin, HIGH);
          delay(500);
          digitalWrite(this->outputPin, LOW);
          this->scheduledWorkEnd = millis();
        }
      }
    }

    bool isAlarm_Overshoot() {
      if (this->percentageToDesired > this->overshootAlarmLevel && !this->alarmSupressor) {
        return true;
      } else {
        return false;
      }
    }

    bool isAlarm_Undershoot() {
      if (this->percentageToDesired < 100.0 && this->startedWorkAt + this->undershootAlarmDelta < millis() && !this->alarmSupressor) {
        return true;
      } else {
        return false;
      }
    }

    bool isActive() {
      return this->activeWork;
    }

    bool isStepping() {
      return this->inStep;
    }

};
