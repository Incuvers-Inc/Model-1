#define BUTTON_SECONDPOLLDELAY 250
#define BUTTON_LOOPCOUNTFASTFORWARD 5
#define BUTTON_FASTFORWARDRATE 10
#define TEMPERATURE_MIN 5.0
#define TEMPERATURE_MAX 75.0
#define TEMPERATURE_DLT 0.5
#define CO2_MIN 0.5
#define CO2_MAX 25.0
#define CO2_DLT 0.1
#define OO_MIN 0.5
#define OO_MAX 21.0
#define OO_DLT 0.1


String CentreStringForDisplay(String prn, int width) {
  String ret = "";
  int padding = ((width - prn.length())/2);

  for (int i = 0; i < padding; i++) {
    ret += " ";
  }
  ret += prn;
  for (int i = 0; i < (width-(padding + prn.length())); i++) {
    ret += " ";
  }

  return ret;
}

char GetIndicator(boolean enabled, boolean stepping, boolean altSymbol) {
  char r = ' ';

  if (enabled && !stepping) {
    if (altSymbol) {
      r = '#';
    } else {
      r = '*';
    }
  } else if (enabled && stepping) {
    if (altSymbol) {
      r = '=';
    } else {
      r = '+';
    }
  }

  return r;
}

boolean IsSameOWAddress(byte addrA[8], byte addrB[8]) {
  boolean isTheSame = true;
  for (int i=0; i<8; i++) {
    if (addrA[i] != addrB[i]) {
      #ifdef DEBUG_GENERAL
        Serial.println("");
        Serial.print(addrA[i], HEX);
        Serial.print(" != ");
        Serial.print(addrB[i], HEX);
      #endif
      isTheSame = false;
      i = 42;
    }
  }
  return isTheSame;
} 

class IncuversUI {
  private:
    LiquidTWI2* lcd;
    IncuversSettingsHandler* incSet;
    int lastButtonState;
    int loopCountButtonState;

    void DisplayLoadingBar() {
      for (int s = 0; s<16; s++) {
        this->lcd->setCursor(s,1);
        this->lcd->print(".");
        delay(MENU_UI_LOAD_DELAY);
      }
    }
    
    void LCDDrawDualLineUI() {  
      int rowI = 0;
      if (incSet->getHeatMode() == 1) {
        lcd->setCursor(0, rowI);
        lcd->print("Temp: ");
        if (incSet->getHeatMode() == 1) {
          lcd->print(incSet->getChamberTemperature(), 1);
          lcd->print("\337C  ");
          if (incSet->getChamberTemperature() < 10.0) {
            lcd->print(" ");
          }
          lcd->print(GetIndicator(incSet->isDoorOn(), incSet->isDoorStepping(), true));
          lcd->print(GetIndicator(incSet->isChamberOn(), incSet->isChamberStepping(), false));
        } else if (incSet->getChamberTemperature() < -20){
          lcd->print(F("Error   "));
        } else {
          lcd->print(F("Disabled"));
        }
        rowI++;
      }
      if (incSet->getCO2Mode() > 0) {
        lcd->setCursor(0, rowI);
        lcd->print(" CO2: ");
        if (incSet->getCO2Mode() > 0) {
          lcd->print(incSet->getCO2Level(), 1);
          lcd->print("%    ");
          if (incSet->getCO2Level() < 10.0) {
            lcd->print(" ");
          }
          lcd->print(GetIndicator(incSet->isCO2Open(), incSet->isCO2Stepping(), false)); 
        } else if (incSet->getCO2Level() < 0){
          lcd->print(F("Error   "));
        } else {
          lcd->print(F("Disabled"));
        }
        rowI++;
      }
      if (incSet->getO2Mode() > 0) {
        lcd->setCursor(0, rowI);
        lcd->print(" O2: ");
        if (incSet->getO2Mode() > 0) {
          lcd->print(incSet->getO2Level(), 1);
          lcd->print("%    ");
          if (incSet->getO2Level() < 10.0) {
            lcd->print(" ");
          }
          lcd->print(GetIndicator(incSet->isO2Open(), incSet->isO2Stepping(), false)); 
        } else if (incSet->getO2Level() < 0){
          lcd->print(F("Error   "));
        } else {
          lcd->print(F("Disabled"));
        }
        rowI++;
      }
    }
    
    void LCDDrawNewUI() {
      /* 0123456789ABCDEF
       * T.*+  CO2+   O2+
       * 35.5  10.5  18.2
       */
      lcd->setCursor(0, 0);
      lcd->print("T.");
      lcd->print(GetIndicator(incSet->isDoorOn(), incSet->isDoorStepping(), true));
      lcd->print(GetIndicator(incSet->isChamberOn(), incSet->isChamberStepping(), false));
      lcd->print("  CO2");
      lcd->print(GetIndicator(incSet->isCO2Open(), incSet->isCO2Stepping(), false)); 
      lcd->print("   O2");
      lcd->print(GetIndicator(incSet->isO2Open(), incSet->isO2Stepping(), false)); 
      
      lcd->setCursor(0, 1);
      if (incSet->getChamberTemperature() > 60.0 || incSet->getChamberTemperature() < -20.0) {
        lcd->print(CentreStringForDisplay("err", 5));
      } else {
        lcd->print(CentreStringForDisplay(String(incSet->getChamberTemperature(), 1), 5));
      }
      if (incSet->getCO2Level() < 0) {
        lcd->print(CentreStringForDisplay("err", 6));
      } else {
        lcd->print(CentreStringForDisplay(String(incSet->getCO2Level(), 1), 6));
      }
      if (incSet->getO2Level() < 0) {
        lcd->print(CentreStringForDisplay("err", 5));
      } else {
        lcd->print(CentreStringForDisplay(String(incSet->getO2Level(), 1), 5));
      }
    }

    String PadToWidth(int source, int intSize) {
      if (String(source).length() >= intSize) {
        return String(source);
      } else {
        return String("0" + PadToWidth(source, intSize - 1));
      }
    }
    
    String ConvertMillisToReadable(long totalMillisCount) {
      long runningAmount;
      int pureMillis = totalMillisCount % 1000;
      runningAmount = floor(totalMillisCount / 1000);
      int seconds = runningAmount % 60;
      runningAmount = floor(runningAmount / 60);
      int minutes = runningAmount % 60;
      runningAmount = floor(runningAmount / 60);
      int hours = runningAmount % 24;
      runningAmount = floor(runningAmount / 24);
      int days = runningAmount % 7;
      int weeks = floor(runningAmount / 7);
      
      String readable = String(PadToWidth(weeks, 2) + "W" + PadToWidth(weeks, 1) + "D" + PadToWidth(hours, 2)+ "H" + PadToWidth(minutes, 2) + "m" + PadToWidth(seconds, 2) + "." + PadToWidth(pureMillis, 4));
      return readable;
    }

    void SerialPrintStatus() {
      char* temp;
      Serial.print(ConvertMillisToReadable(millis()));
      Serial.print(F(" ID "));
      Serial.print(incSet->getSerial());
      Serial.print(F(" TC "));
      sprintf(temp, "%04.2e", incSet->getChamberTemperature());
      Serial.print(temp);
      Serial.print(F(" TD "));
      sprintf(temp, "%04.2e", incSet->getDoorTemperature());
      Serial.print(temp);
      Serial.print(F(" CO "));
      sprintf(temp, "%04.2e", incSet->getCO2Level());
      Serial.print(temp);
      Serial.print(F(" OO "));
      sprintf(temp, "%04.2e", incSet->getO2Level());
      Serial.print(temp);
      Serial.print(F(" AP "));
      Serial.print(GetIndicator(incSet->isDoorOn(), incSet->isDoorStepping(), false));
      Serial.print(GetIndicator(incSet->isChamberOn(), incSet->isChamberStepping(), false));
      Serial.print(GetIndicator(incSet->isCO2Open(), incSet->isCO2Stepping(), false));
      Serial.print(GetIndicator(incSet->isO2Open(), incSet->isO2Stepping(), false));
      Serial.print(F(" OA "));
      Serial.print("");
      Serial.print("");
      Serial.println();
    }
    
    // For debugging only
    void SerialPrintResults() {
      String blankString = F("");
      String onString = F("on");
      String yesString = F("yes");
      String divString = F("|");
      Serial.println(F("-----------------------------------------------"));
      Serial.print(F("Time in operation "));
      Serial.println(ConvertMillisToReadable(millis()));
      // Header
     /* Serial.print(CentreStringForDisplay(F(""),10));
      Serial.print(divString);
      Serial.print(CentreStringForDisplay(F("HEAT"),16));
      Serial.print(divString);
      Serial.print(CentreStringForDisplay(F("FAN"),16));
      Serial.print(divString);
      Serial.print(CentreStringForDisplay(F("CO2"),16));
      Serial.print(divString);
      Serial.println(CentreStringForDisplay(F("O2"),16));
      // Enabled
      Serial.print(CentreStringForDisplay(F("Enabled"),10));
      Serial.print(divString);
      if (incSet->getHeatMode() > 0) {
        Serial.print(CentreStringForDisplay(yesString,16));
      } else {
        Serial.print(CentreStringForDisplay(blankString,16));
      }
      Serial.print(divString);
      if (incSet->getFanMode() > 0) {
        Serial.print(CentreStringForDisplay(yesString,16));
      } else {
        Serial.print(CentreStringForDisplay(blankString,16));
      }
      Serial.print(divString);
      if (incSet->getCO2Mode() > 0) {
        Serial.print(CentreStringForDisplay(yesString,16));
      } else {
        Serial.print(CentreStringForDisplay(blankString,16));
      }
      Serial.print(divString);
      if (incSet->getO2Mode() > 0) {
        Serial.println(CentreStringForDisplay(yesString,16));
      } else {
        Serial.println(CentreStringForDisplay(blankString,16));
      }
     /* // setpoints
      Serial.print(CentreStringForDisplay(F("SetPoint"),10));
      Serial.print(divString);
      if (incSet->getHeatMode() > 0) {
        Serial.print(incSet->getTemperatureSetPoint());
      } else {
        Serial.print(CentreStringForDisplay(blankString,16));
      }
      Serial.print(divString);
      if (incSet->getFanMode() > 0) {
        Serial.print(CentreStringForDisplay(blankString,16));
      } else {
        Serial.print(CentreStringForDisplay(blankString,16));
      }
      Serial.print(divString);
      if (incSet->getCO2Mode() > 0) {
        Serial.print(incSet->getCO2SetPoint());
      } else {
        Serial.print(CentreStringForDisplay(blankString,16));
      }
      Serial.print(divString);
      if (incSet->getO2Mode() > 0) {
        Serial.println(incSet->getO2SetPoint());
      } else {
        Serial.println(CentreStringForDisplay(blankString,16));
      }
      // Readings
      Serial.print(CentreStringForDisplay(F("Readings"),10));
      Serial.print(divString);
      if (incSet->getHeatMode() > 0) {
        Serial.print(CentreStringForDisplay(String(incSet->getChamberTemperature()),16));
      } else {
        Serial.print(CentreStringForDisplay(blankString,16));
      }
      Serial.print(divString);
      if (incSet->getFanMode() > 0) {
        Serial.print(CentreStringForDisplay(blankString,16));
      } else {
        Serial.print(CentreStringForDisplay(blankString,16));
      }
      Serial.print(divString);
      if (incSet->getCO2Mode() > 0) {
        Serial.print(CentreStringForDisplay(String(incSet->getCO2Level()),16));
      } else {
        Serial.print(CentreStringForDisplay(blankString,16));
      }
      Serial.print(divString);
      if (incSet->getO2Mode() > 0) {
        Serial.println(CentreStringForDisplay(String(incSet->getO2Level()),16));
      } else {
        Serial.println(CentreStringForDisplay(blankString,16));
      }
      // Working
      Serial.print(CentreStringForDisplay(F("Working"),10));
      Serial.print(divString);
      if (incSet->getHeatMode() > 0) {
        Serial.print(CentreStringForDisplay(String(GetIndicator(incSet->isDoorOn(), incSet->isDoorStepping(), false)+GetIndicator(incSet->isChamberOn(), incSet->isChamberStepping(), true)),16));
      } else {
        Serial.print(CentreStringForDisplay(blankString,16));
      }
      Serial.print(divString);
      if (incSet->getFanMode() > 0) {
        Serial.print(CentreStringForDisplay(blankString,16));
      } else {
        Serial.print(CentreStringForDisplay(blankString,16));
      }
      Serial.print(divString);
      if (incSet->getCO2Mode() > 0) {
        Serial.print(CentreStringForDisplay(String(GetIndicator(incSet->isCO2Open(), incSet->isCO2Stepping(), false)),16));
      } else {
        Serial.print(CentreStringForDisplay(blankString,16));
      }
      Serial.print(divString);
      if (incSet->getO2Mode() > 0) {
        Serial.println(CentreStringForDisplay(String(GetIndicator(incSet->isO2Open(), incSet->isO2Stepping(), false)),16));
      } else {
        Serial.println(CentreStringForDisplay(blankString,16));
      }
    */
      Serial.println("");
    }
    void AlarmOrchestrator() {
    /*  if (statusHolder.AlarmTempSensorMalfunction == true) {
        // There's not much we can do for this one but turn off/disable the heating system.
        ShutdownHeatSystem();
        settingsHolder.heatMode = 0;
      }
    
      if (statusHolder.AlarmCO2SensorMalfunction == true) {
        // There's not much we can do for this one but turn off/disable the heating system.
        ShutdownCO2System();
        settingsHolder.CO2Mode = 0;
      }
    
      if (statusHolder.AlarmHeatDoorOverTemp == true || 
          /*statusHolder.AlarmHeatDoorUnderTemp == true || 
      //    statusHolder.AlarmHeatChamberOverTemp == true || 
       //   statusHolder.AlarmHeatChamberUnderTemp == true) {
        ShutdownHeatSystem();
        ShutdownCO2System(); 
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temp. Error");
        lcd.setCursor(0, 1);
        lcd.print(statusHolder.TempChamber, 1);
        lcd.print("\337C | ");
        lcd.print(statusHolder.TempDoor, 1);
        lcd.print("\337C");
        
        while (GetButtonState() != 3){
          Wire.beginTransmission(0x20); // Connect to chip
          Wire.write(0x13);             // Address port B
          Wire.write(0x01);                // Sound On
          Wire.endTransmission();       // Close connection
          delay(500);
          Wire.beginTransmission(0x20); // Connect to chip
          Wire.write(0x13);             // Address port B
          Wire.write(0x00);                // Sound On
          Wire.endTransmission();       // Close connection
          delay(500);
        }   
      }
    
      if (statusHolder.AlarmCO2OverSaturation == true || 
          statusHolder.AlarmCO2UnderSaturation == true) {
        ShutdownHeatSystem();
        ShutdownCO2System(); 
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("CO2 Error");
        lcd.setCursor(0, 1);
        lcd.print("CO2: ");
        lcd.print(statusHolder.CO2Reading, 1);
        lcd.print("% ");
        
        while (GetButtonState() != 3){
          Wire.beginTransmission(0x20); // Connect to chip
          Wire.write(0x13);             // Address port B
          Wire.write(0x01);                // Sound On
          Wire.endTransmission();       // Close connection
          delay(500);
          Wire.beginTransmission(0x20); // Connect to chip
          Wire.write(0x13);             // Address port B
          Wire.write(0x00);                // Sound On
          Wire.endTransmission();       // Close connection
          delay(500);
        }    
      }
      */
    }
    
    int PollButton() {
        Wire.beginTransmission(0x20); // Connect to chip
        Wire.write(0x12);             // Address port A
        Wire.endTransmission();       // Close connection
        Wire.requestFrom(0x20, 1);    // Request one Byte
        return(Wire.read());          // Return the state 
                                      //  0 = no buttons pushed
                                      //  1 = upper button pushed
                                      //  2 = lower button pushed
                                      //  3 = both buttons pushed
    }
    
    int GetButtonState() {
      // instead of reading the button once which can cause misreads during a button change operation, if the first reading is not 0, take two more readings a few moments apart to verify what the user wants to do.
      int c, s, r = 0;
    
      c = PollButton();
    
      if (c != 0) {
        if (c == lastButtonState) {
          // the button is in the same state as it was at the previous polling,
          r = c;
          loopCountButtonState++;
        } else {
          loopCountButtonState = 0;
          // let's do an extra polling, to make sure we get the right value.
          delay(BUTTON_SECONDPOLLDELAY);
          s = PollButton();
          if (c == 3 || s == 3 || (c == 1 && s == 2) || (c == 2 && s == 1)) {
            r = 3;
          } else if (c == 2 || s == 2) {
            r = 2;
          } else if (c == 1 || s == 1) {
            r = 1;
          }
        }
        lastButtonState = r;
      } else {
        lastButtonState = 0;
        loopCountButtonState = 0;
      }
      
      return r;
    }

    int GetNewGasMode(int mode, int altMode) {
      if (mode != 0) {
        return 0; // gas is on, turn it off.
      } else if (mode == 0 && (altMode == 0 || (altMode == 2 && incSet->isSecondGasSensorSupported()))) {
        return 1; // both gases are off, can start with first
      } else if (mode == 0 && altMode == 1 && incSet->isSecondGasSensorSupported()) {
        return 2;
      } else {
        return 0; // default to off as we don't have 'room'
      }
    }

    void DoFeatureSet(int feat)
    {
      boolean doLoop = true;
      boolean redraw = true;
      int userInput;
      int mode;
      int altMode;
      int countOfEnabledGases;
      String tag;
      String onTag;
      
      while (doLoop) {
        if (redraw) {
          switch (feat) {
            case 1: // heat
              mode = incSet->getHeatMode();
              tag = F("Heat: ");
              onTag = F("Enabled ");
              break;
            case 2: // fan
              mode = incSet->getFanMode();
              tag = F("Fan: ");
              onTag = F("Always  ");
              break;
            case 3: // CO2
              mode = incSet->getCO2Mode();
              altMode = incSet->getO2Mode();
              tag = F("CO2: ");
              if (mode == 1) { onTag = F("First   "); }
              if (mode == 2) { onTag = F("Second  "); }
              break;
            case 4: // O2
              mode = incSet->getO2Mode();
              altMode = incSet->getCO2Mode();
              tag = F("O2: ");
              if (mode == 1) { onTag = F("First   "); }
              if (mode == 2) { onTag = F("Second  "); }
              break;
          }
          lcd->clear();
          lcd->setCursor(0, 0);
          lcd->print(tag);
          if (mode != 0) {
            lcd->print(onTag);
          } else {
            lcd->print(F("Disabled"));
          }
          delay(MENU_UI_POST_DELAY);
        }
       
        userInput = GetButtonState();
        
        switch (userInput) {
          case 0:
            delay(100);
            redraw=false;
            break;
          case 1:
            // Toggle Heat
            switch (feat) {
              case 1: // heat
                if (incSet->getHeatMode() == 0) {
                  incSet->setHeatMode(1);
                } else {
                  incSet->setHeatMode(0);
                }
                break;
              case 2: // fan
                if (incSet->getFanMode() == 0) {
                  incSet->setFanMode(4);
                } else {
                  incSet->setFanMode(0);
                }
                break;
              case 3: // CO2
                if (incSet->getCO2Mode() == 0) {
                  incSet->setCO2Mode(GetNewGasMode(mode, altMode));
                } else {
                  incSet->setCO2Mode(0);
                }
                break;
              case 4: // O2
                if (incSet->getO2Mode() == 0) {
                  incSet->setO2Mode(GetNewGasMode(mode, altMode));
                } else {
                  incSet->setO2Mode(0);
                }
                break;
            }
            
            redraw = true;
            break;
          case 2:
            // Do nothing
            break;
          case 3:
            // exit
            doLoop=false;
            break;
        }
      }
    }

    void DoSaveSettings() {
      incSet->PerformSaveSettings();
      lcd->clear();
      lcd->setCursor(0, 0);
      lcd->print(F("Settings saved."));
      lcd->setCursor(0, 1);
      lcd->print(F("Exiting setup..."));  
      incSet->CheckSettings();
      delay(MENU_UI_POST_DELAY * 4);
      lcd->clear();
    }

    void DrawVariableMenuPage(String title, float value) {
      lcd->clear();
      lcd->setCursor(0, 0);
      lcd->print(title);
      lcd->setCursor(2, 1);
      lcd->print(value, 1);
      lcd->setCursor(15, 0);
      lcd->print("+");
      lcd->setCursor(15, 1);
      lcd->print("-");
      delay(MENU_UI_POST_DELAY);
    }

    void AdjustTempSetting(boolean upWards) {
      float newSet = incSet->getTemperatureSetPoint();
      if (upWards && loopCountButtonState >= BUTTON_LOOPCOUNTFASTFORWARD) {
        #ifdef DEBUG_UI
          Serial.println(F("++"));
        #endif
        newSet = newSet + (TEMPERATURE_DLT * BUTTON_FASTFORWARDRATE);
      } else if (upWards && loopCountButtonState < BUTTON_LOOPCOUNTFASTFORWARD) {
        #ifdef DEBUG_UI
          Serial.println(F("+"));
        #endif
        newSet = newSet + TEMPERATURE_DLT;
      } else if (!upWards && loopCountButtonState >= BUTTON_LOOPCOUNTFASTFORWARD) {
        #ifdef DEBUG_UI
          Serial.println(F("--"));
        #endif
        newSet = newSet - (TEMPERATURE_DLT * BUTTON_FASTFORWARDRATE);
      } else if (!upWards && loopCountButtonState < BUTTON_LOOPCOUNTFASTFORWARD) {
        #ifdef DEBUG_UI
          Serial.println(F("-"));
        #endif
        newSet = newSet - TEMPERATURE_DLT;
      }
      if (newSet < TEMPERATURE_MIN) { 
        newSet = TEMPERATURE_MIN; 
      }
      if (newSet > TEMPERATURE_MAX) { 
        newSet = TEMPERATURE_MAX; 
      }
      incSet->setTemperatureSetPoint(newSet);
    }

    void AdjustCO2Setting(boolean upWards) {
      float newSet = incSet->getCO2SetPoint();
      if (upWards && loopCountButtonState >= BUTTON_LOOPCOUNTFASTFORWARD) {
        newSet = newSet + (CO2_DLT * BUTTON_FASTFORWARDRATE);
      } else if (upWards && loopCountButtonState < BUTTON_LOOPCOUNTFASTFORWARD) {
        newSet = newSet + CO2_DLT;
      } else if (!upWards && loopCountButtonState >= BUTTON_LOOPCOUNTFASTFORWARD) {
        newSet = newSet - (CO2_DLT * BUTTON_FASTFORWARDRATE);
      } else if (!upWards && loopCountButtonState < BUTTON_LOOPCOUNTFASTFORWARD) {
        newSet = newSet - CO2_DLT;
      }
      if (newSet < CO2_MIN) { 
        newSet = CO2_MIN; 
      }
      if (newSet > CO2_MAX) { 
        newSet = CO2_MAX; 
      }
      incSet->setCO2SetPoint(newSet);
    }

    void AdjustO2Setting(boolean upWards) {
      float newSet = incSet->getO2SetPoint();
      if (upWards && loopCountButtonState >= BUTTON_LOOPCOUNTFASTFORWARD) {
        newSet = newSet + (OO_DLT * BUTTON_FASTFORWARDRATE);
      } else if (upWards && loopCountButtonState < BUTTON_LOOPCOUNTFASTFORWARD) {
        newSet = newSet + OO_DLT;
      } else if (!upWards && loopCountButtonState >= BUTTON_LOOPCOUNTFASTFORWARD) {
        newSet = newSet - (OO_DLT * BUTTON_FASTFORWARDRATE);
      } else if (!upWards && loopCountButtonState < BUTTON_LOOPCOUNTFASTFORWARD) {
        newSet = newSet - OO_DLT;
      }
      if (newSet < OO_MIN) { 
        newSet = OO_MIN; 
      }
      if (newSet > OO_MAX) { 
        newSet = OO_MAX; 
      }
      incSet->setO2SetPoint(newSet);
    }
    
    void DoVariableAdjust(int screenId) {
      boolean doLoop = true;
      boolean redraw = true;
      int userInput;
    
      while (doLoop) {
        switch (screenId) {
          case 1 :
            if (redraw) {
              DrawVariableMenuPage(F("Temperature:"), incSet->getTemperatureSetPoint());
              redraw = false;
            } else if (userInput == 1 || userInput == 2) {
              if (userInput == 1) { AdjustTempSetting(true); } else { AdjustTempSetting(false); }
              redraw = true;
              delay(MENU_UI_POST_DELAY);
            }
            break;
          case 2 :
            if (redraw) {
              DrawVariableMenuPage(F("CO2:"), incSet->getCO2SetPoint());
              redraw = false;
            } else if (userInput == 1 || userInput == 2) {
              if (userInput == 1) { AdjustCO2Setting(true); } else { AdjustCO2Setting(false); }
              redraw = true;
              delay(MENU_UI_POST_DELAY);
            }
            break;
          case 3 :
            if (redraw) {
              DrawVariableMenuPage(F("O2:"), incSet->getO2SetPoint());
              redraw = false;
            } else if (userInput == 1 || userInput == 2) {
              if (userInput == 1) { AdjustO2Setting(true); } else { AdjustO2Setting(false); }
              redraw = true;
              delay(MENU_UI_POST_DELAY);
            }
            break;
          }
        
       
        userInput = GetButtonState();
        
        switch (userInput) {
          case 0:
            redraw=false;
            break;
          case 1:
            // Will take care of it on the next loop run
            break;
          case 2:
            // Will take care of it on the next loop run
            break;
          case 3:
            // exit
            doLoop=false;
            break;
        }
      }
    }

    void DrawMainMenuPage(String lineOne, String lineTwo, boolean isStandardPage, String optOne, String optTwo) {
      lcd->clear();
      lcd->setCursor(0, 0);
      lcd->print(lineOne);
      if (lineTwo.length() > 0) {
        lcd->setCursor(0, 1);
        lcd->print(lineTwo);
      }
      if (isStandardPage) {
        lcd->setCursor(13, 0);
        lcd->print("Set");
        lcd->setCursor(12, 1);
        lcd->print("Next");
      } else {
        lcd->setCursor((16 - optOne.length()), 0);
        lcd->print(optOne);
        lcd->setCursor((16 - optTwo.length()), 1);
        lcd->print(optTwo);
      }
    }
    
    void SetupLoop() {
      boolean doLoop = true;
      boolean redraw = true;
      int screen = 0;
      int userInput;
      lcd->clear();
      lcd->setCursor(0, 0);
      lcd->print(F("Incuvers Setup"));
      DisplayLoadingBar();
      delay(MENU_UI_REDRAW_DELAY);
      while (doLoop) {
        if (screen < 1 || screen > 11) {
          screen = 1; // go to the first screen
        }

        if (screen == 1 && incSet->getHeatMode() == 0) {
          screen++;
        }
        if (screen == 2 && incSet->getCO2Mode() == 0) {
          screen++;
        }
        if (screen == 3 && incSet->getO2Mode() == 0) {
          screen++;
        }
        
        switch (screen) {
          case 1:
            if (redraw) {
              DrawMainMenuPage("Set Temp.", "", true, "", "");
            } else if (userInput == 1) {
              DoVariableAdjust(1);
              delay(MENU_UI_POST_DELAY);
              redraw = true;
              userInput = 0;
            }
            break;
          case 2:
            if (redraw) {
              DrawMainMenuPage("Set CO2", "", true, "", "");
            } else if (userInput == 1) {
              DoVariableAdjust(2);
              delay(MENU_UI_POST_DELAY);
              redraw = true;
              userInput = 0;
            }
            break;
          case 3:
            if (redraw) {
              DrawMainMenuPage("Set O2", "", true, "", "");
            } else if (userInput == 1) {
              DoVariableAdjust(3);
              delay(MENU_UI_POST_DELAY);
              redraw = true;
              userInput = 0;
            }
            break;
          case 4:
            if (redraw) {
              DrawMainMenuPage("Settings", "", false, "Save", "Advanced");
            } else if (userInput == 1) {
              DoSaveSettings();
              delay(MENU_UI_POST_DELAY);
              doLoop = false;
              userInput = 0;
            }
            break;
          case 5:
            if (redraw) {
              DrawMainMenuPage("Heating", "", true, "", "");
            } else if (userInput == 1) {
              DoFeatureSet(1);
              delay(MENU_UI_POST_DELAY);
              redraw = true;
              userInput = 0;
            }
            break;
          case 6:
            if (redraw) {
              DrawMainMenuPage("Fan Mode", "", true, "", "");
            } else if (userInput == 1) {
              DoFeatureSet(2);
              delay(MENU_UI_POST_DELAY);
              redraw = true;
              userInput = 0;
            }
            break;
          case 7:
            if (redraw) {
              DrawMainMenuPage("CO2", "", true, "", "");
            } else if (userInput == 1) {
              DoFeatureSet(3);
              delay(MENU_UI_POST_DELAY);
              redraw = true;
              userInput = 0;
            }
            break;
          case 8:
            if (redraw) {
              DrawMainMenuPage("Oxygen", "", true, "", "");
            } else if (userInput == 1) {
              DoFeatureSet(4);
              delay(MENU_UI_POST_DELAY);
              redraw = true;
              userInput = 0;
            }
            break;
          case 9:
            if (redraw) {
              DrawMainMenuPage("Information", "", true, "", "");
            } else if (userInput == 1) {
              //doTempSensorAssign();
              delay(MENU_UI_POST_DELAY);
              redraw = true;
              userInput = 0;
            }
            break;  
          case 10:
            if (redraw) {
              DrawMainMenuPage("Defaults", "", false, "Reset", "Next");
            } else if (userInput == 1) {
              delay(MENU_UI_POST_DELAY);
              //doResetToDefaults();
              lcd->clear();
              lcd->setCursor(0, 0);
              lcd->print(F("Reset to default"));
              DisplayLoadingBar();
              doLoop = false;
              userInput = 0;
            }
            break;
          case 11:
            if (redraw) {
              DrawMainMenuPage("Settings", "", false, "Save", "Basic");
            } else if (userInput == 1) {
              DoSaveSettings();
              delay(MENU_UI_POST_DELAY);
              doLoop = false;
              userInput = 0;
            }
            break;
        }
        
        //delay(MENU_UI_REDRAW_DELAY);
              
        userInput = GetButtonState();
        switch (userInput) {
          case 0:
            delay(MENU_UI_LOAD_DELAY);
            redraw=false;
            break;
          case 1:
            // Will skip this now and get it on the next loop.
            break;
          case 2:
            // Next
            screen++;
            lcd->clear();
            delay(MENU_UI_LOAD_DELAY);
            redraw = true;
            break;
          case 3:
            // exit
            lcd->clear();
            lcd->setCursor(0, 0);
            lcd->print(F("Exiting setup..."));
            DisplayLoadingBar();
            incSet->CheckSettings();
            doLoop=false;
            lcd->clear();
            break;
        } 
      }
    }
    
  public:
    void SetupUI() {
      this->lcd = new LiquidTWI2(0);
      this->lcd->setMCPType(LTI_TYPE_MCP23017);
      this->lcd->begin(16, 2);
  
      Wire.begin(); // wake up I2C bus
      Wire.beginTransmission(0x20);
      Wire.write(0x00); // IODIRA register
      Wire.write(0x01); // set all of port A to inputs
      Wire.endTransmission();
    }

    void AttachSettings(IncuversSettingsHandler* iSettings) {
      this->incSet = iSettings;
    }
  
    void DisplayStartup() {
      this->lcd->setCursor(0,0);
      this->lcd->print(F("Incuvers Model 1"));
      this->lcd->setCursor(0,1);
      this->lcd->print(CentreStringForDisplay(F("V1.9"),16));
      delay(1000);
      this->lcd->clear();
  
      String longDebugDesc = "";
      String shortDebugDesc = "";
  
      #ifdef DEBUG_GENERAL
        longDebugDesc += F("General, ");
        shortDebugDesc += "G";
      #endif
  
      #ifdef DEBUG_SERIAL
        longDebugDesc += F("Serial, ");
        shortDebugDesc += "S";
      #endif
      
      #ifdef DEBUG_EEPROM
        longDebugDesc += F("EEPROM, ");
        shortDebugDesc += "E";
      #endif

      #ifdef DEBUG_UI
        longDebugDesc += F("UI, ");
        shortDebugDesc += "U";
      #endif

      #ifdef DEBUG_CO2
        longDebugDesc += F("CO2, ");
        shortDebugDesc += "C";
      #endif
      
      #ifdef DEBUG_O2
        longDebugDesc += F("O2, ");
        shortDebugDesc += "O";
      #endif
      
      #ifdef DEBUG_TEMP
        longDebugDesc += F("Temperature, ");
        shortDebugDesc += "T";
      #endif
      
      if (longDebugDesc.length() > 0) {
        Serial.println(F("Debug build: "));
        Serial.println(longDebugDesc);
      }
  
      if (shortDebugDesc.length() > 0) {
        this->lcd->setCursor(0,0);
        this->lcd->print(CentreStringForDisplay(String("Debug: " + shortDebugDesc),16));
        this->DisplayLoadingBar();
        this->lcd->clear();
      }
    }
  
    void DisplayRunMode(int runMode) {
      lcd->setCursor(0,0);
      switch (runMode) {
        case 0 :
          lcd->print(CentreStringForDisplay(F("Uninitialized"),16));
          break;
        case 1 :
          lcd->print(CentreStringForDisplay(F("Saved Settings"),16));
          break;
        case 2 :
          lcd->print(CentreStringForDisplay(F("Default Settings"),16));
          break;
        case 3 :
          lcd->print(CentreStringForDisplay(F("Bad Config!"),16));
          break;
        case 4 :
          lcd->print(CentreStringForDisplay(F("Updating"),16));
          break;
      }
      this->DisplayLoadingBar();
      lcd->clear();
    }
  
    void LCDDrawDefaultUI() {
      if (incSet->getPersonalityCount() == 3) {
        LCDDrawNewUI();
      }
      if (incSet->getPersonalityCount() < 3) {
        LCDDrawDualLineUI();
      }
    }

    void EnterSetupMode() {
      incSet->MakeSafeState();
      SetupLoop();
    }
    
    void DoTick() {
      LCDDrawDefaultUI();

      int userInput = GetButtonState();
      if (userInput == 3) {
        EnterSetupMode();  
      }
   
      #ifdef DEBUG_GENERAL
        SerialPrintResults();
      #endif
      #ifndef DEBUG_GENERAL
        SerialPrintStatus();
      #endif
    }
    
};


/*
String GetFanSettingString() {
  String ret;
  
  switch(settingsHolder.fanMode) {
    case 0:
      ret = F("Öff");
      break;
    case 1:
      ret = F("Heat + 30s");
      break;
    case 2:
      ret = F("Heat + 60s");
      break;
    case 3:
      ret = F("Heat + 50%");
      break;
    case 4:
      ret = F("Ön");
      break;
  }
  return ret;
}
*/

