#define MENU_UI_LOAD_DELAY 75
#define MENU_UI_POST_DELAY 250
#define MENU_UI_REDRAW_DELAY 500
#define BUTTON_SECONDPOLLDELAY 250
#define BUTTON_LOOPCOUNTFASTFORWARD 5
#define BUTTON_FASTFORWARDRATE 10
#define TEMPERATURE_MIN 5.0
#define TEMPERATURE_MAX 75.0
#define TEMPERATURE_DLT 0.5
#define CO2_MIN 0.1
#define CO2_MAX 25.0
#define CO2_DLT 0.1
#define OO_MIN 0.0
#define OO_MAX 21.0
#define OO_DLT 0.1

class IncuversUI {
  private:
    LiquidTWI2* lcd;
    IncuversSettingsHandler* incSet;
    int lastButtonState;
    int loopCountButtonState;
    unsigned long lastRefresh;
    
    void DisplayLoadingBar() {
      for (int s = 0; s<16; s++) {
        this->lcd->setCursor(s,1);
        this->lcd->print(".");
        delay(MENU_UI_LOAD_DELAY);
      }
    }
    
    void LCDDrawDualLineUI() {
      #ifdef DEBUG_UI
      Serial.print(F("UI::LCDDrawDualLineUI - "));
      #endif
        
      int rowI = 0;
      if (incSet->getHeatMode() == 1) {
        #ifdef DEBUG_UI
        Serial.print(F("Heat "));
        #endif
        
        lcd->setCursor(0, rowI);
        lcd->print("Temp: ");
        if (incSet->getHeatMode() == 1) {
          lcd->print(incSet->getHeatModule()->getChamberTemperature(), 1);
          lcd->print("\337C  ");
          if (incSet->getHeatModule()->getChamberTemperature() < 10.0) {
            lcd->print(" ");
          }
          lcd->print(GetIndicator(incSet->getHeatModule()->isDoorOn(), incSet->getHeatModule()->isDoorStepping(), true, false));
          lcd->print(GetIndicator(incSet->getHeatModule()->isChamberOn(), incSet->getHeatModule()->isChamberStepping(), false, false));
        } else if (incSet->getHeatModule()->getChamberTemperature() < -20){
          lcd->print(F("Error   "));
        } else {
          lcd->print(F("Disabled"));
        }
        rowI++;
      }
      if (incSet->getCO2Mode() > 0) {
        #ifdef DEBUG_UI
        Serial.print(F("CO2 "));
        #endif
        lcd->setCursor(0, rowI);
        lcd->print(" CO2: ");
        if (incSet->getCO2Mode() > 0 && incSet->getCO2Module()->getCO2Level() >= 0) {
          lcd->print(incSet->getCO2Module()->getCO2Level(), 1);
          lcd->print("%    ");
          if (incSet->getCO2Module()->getCO2Level() < 10.0) {
            lcd->print(" ");
          }
          lcd->print(GetIndicator(incSet->getCO2Module()->isCO2Open(), incSet->getCO2Module()->isCO2Stepping(), false, false)); 
        } else if (incSet->getCO2Module()->getCO2Level() < 0){
          lcd->print(F("Error   "));
        } else {
          lcd->print(F("Disabled"));
        }
        rowI++;
      }
      if (incSet->getO2Mode() > 0) {
        #ifdef DEBUG_UI
        Serial.print(F("O2 "));
        #endif
        lcd->setCursor(0, rowI);
        lcd->print("  O2: ");
        if (incSet->getO2Mode() > 0 && incSet->getO2Module()->getO2Level() >= 0) {
          lcd->print(incSet->getO2Module()->getO2Level(), 1);
          lcd->print("%    ");
          if (incSet->getO2Module()->getO2Level() < 10.0) {
            lcd->print(" ");
          }
          lcd->print(GetIndicator(incSet->getO2Module()->isNOpen(), incSet->getO2Module()->isNStepping(), false, false)); 
        } else if (incSet->getO2Module()->getO2Level() < 0){
          lcd->print(F("Error   "));
        } else {
          lcd->print(F("Disabled"));
        }
        rowI++;
      }
      if (incSet->getLightMode() > 0) {
        #ifdef DEBUG_UI
        Serial.print(F("Light "));
        #endif
        lcd->setCursor(0, rowI);
        lcd->print(incSet->getLightModule()->GetOldUIDisplay());
        rowI++;
      }
      #ifdef DEBUG_UI
      Serial.println(F(""));
      #endif
    }
    
    void LCDDrawNewUI() {
      /* 0123456789ABCDEF
       * T.*+  CO2+   O2+
       * 35.5  10.5  18.2
       */

      // TODO: Fix this UI display to support lighting.
      lcd->setCursor(0, 0);
      lcd->print("T.");
      lcd->print(GetIndicator(incSet->getHeatModule()->isDoorOn(), incSet->getHeatModule()->isDoorStepping(), true, false));
      lcd->print(GetIndicator(incSet->getHeatModule()->isChamberOn(), incSet->getHeatModule()->isChamberStepping(), false, false));
      lcd->print("  CO2");
      lcd->print(GetIndicator(incSet->getCO2Module()->isCO2Open(), incSet->getCO2Module()->isCO2Stepping(), false, false)); 
      lcd->print("   O2");
      lcd->print(GetIndicator(incSet->getO2Module()->isNOpen(), incSet->getO2Module()->isNStepping(), false, false)); 
      
      lcd->setCursor(0, 1);
      if (incSet->getHeatModule()->getChamberTemperature() > 60.0 || incSet->getHeatModule()->getChamberTemperature() < -20.0) {
        lcd->print(CentreStringForDisplay("err", 5));
      } else {
        lcd->print(CentreStringForDisplay(String(incSet->getHeatModule()->getChamberTemperature(), 1), 5));
      }
      if (incSet->getCO2Module()->getCO2Level() < 0) {
        lcd->print(CentreStringForDisplay("err", 6));
      } else {
        lcd->print(CentreStringForDisplay(String(incSet->getCO2Module()->getCO2Level(), 1), 6));
      }
      if (incSet->getO2Module()->getO2Level() < 0) {
        lcd->print(CentreStringForDisplay("err", 5));
      } else {
        lcd->print(CentreStringForDisplay(String(incSet->getO2Module()->getO2Level(), 1), 5));
      }
    }

    void SerialPrintStatus() {
      #ifdef SHOWSERIALSTATUS
        Serial.println(incSet->GenerateStatusLine(false));
      #endif
    }
    
    void AlarmOrchestrator() {
      if ((incSet->getHeatModule()->isAlarmed() || incSet->getCO2Module()->isAlarmed() || incSet->getO2Module()->isAlarmed())/* && incSet->getAlarmMode() == 2*/) {
        incSet->MakeSafeState();
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
        incSet->ResetAlarms();
      }
    }
    
    int PollButton() {
      int readVal;
      int returnVal = 0;
      Wire.beginTransmission(0x20); // Connect to chip
      Wire.write(0x12);             // Address port A
      Wire.endTransmission();       // Close connection
      Wire.requestFrom(0x20, 1);    // Request one Byte
      readVal = Wire.read();
      returnVal = readVal & 3; 
      #ifdef DEBUG_UI
        Serial.print(F("Poll: "));
        Serial.print(readVal);
        Serial.print(F(" --> "));
        Serial.println(returnVal);
      #endif
      
      return(returnVal);          // Return the state 
                                    //  0 = no buttons pushed
                                    //  1 = upper button pushed
                                    //  2 = lower button pushed
                                    //  3 = both buttons pushed
    }
    
    int GetButtonState() {
      // instead of reading the button once which can cause misreads during a button change operation, if the first reading is not 0, take two more readings a few moments apart to verify what the user wants to do.
      int c = 0, s = 0, r = 0;
    
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

    void DoFeatureToggle(int feat)
    {
      boolean doLoop = true;
      boolean redraw = true;
      int userInput;
      int mode;
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
              tag = F("CO2: ");
              if (incSet->getCO2Mode() == 1) { onTag = F("Monitor "); }
              if (incSet->getCO2Mode() == 2) { onTag = F("Maintain"); }
              break;
            case 4: // O2
              mode = incSet->getO2Mode();
              tag = F("O2: ");
              if (incSet->getO2Mode() == 1) { onTag = F("Monitor "); }
              if (incSet->getO2Mode() == 2) { onTag = F("Maintain"); }
              break;
            case 5: // Light
              mode = incSet->getLightMode();
              tag = F("Light: ");
              if (incSet->getLightMode() == 1) { onTag = F("Int."); }
              if (incSet->getLightMode() == 2) { onTag = F("Ext."); }
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
                  incSet->setCO2Mode(1);
                } else if (incSet->getCO2Mode() == 1) {
                  incSet->setCO2Mode(2);
                } else {
                  incSet->setCO2Mode(0);
                }
                break;
              case 4: // O2
                if (incSet->getO2Mode() == 0) {
                  incSet->setO2Mode(1);
                } else if (incSet->getO2Mode() == 1) {
                  incSet->setO2Mode(2);
                } else {
                  incSet->setO2Mode(0);
                }
                break;
              case 5: // Light
                if (incSet->getLightMode() == 0) {
                  incSet->setLightMode(1);
                } else if (incSet->getLightMode() == 1) {
                  incSet->setLightMode(2);
                } else {
                  incSet->setLightMode(0);
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
              DrawVariableMenuPage(F("Temperature"), incSet->getTemperatureSetPoint());
              redraw = false;
            } else if (userInput == 1 || userInput == 2) {
              if (userInput == 1) { AdjustTempSetting(true); } else { AdjustTempSetting(false); }
              redraw = true;
              delay(MENU_UI_REDRAW_DELAY);
            }
            break;
          case 2 :
            if (redraw) {
              DrawVariableMenuPage(F("CO2 %"), incSet->getCO2SetPoint());
              redraw = false;
            } else if (userInput == 1 || userInput == 2) {
              if (userInput == 1) { AdjustCO2Setting(true); } else { AdjustCO2Setting(false); }
              redraw = true;
              delay(MENU_UI_REDRAW_DELAY);
            }
            break;
          case 3 :
            if (redraw) {
              DrawVariableMenuPage(F("O2 %"), incSet->getO2SetPoint());
              redraw = false;
            } else if (userInput == 1 || userInput == 2) {
              if (userInput == 1) { AdjustO2Setting(true); } else { AdjustO2Setting(false); }
              redraw = true;
              delay(MENU_UI_REDRAW_DELAY);
            }
            break;
          }
        
        if (!redraw) {
          userInput = GetButtonState();
        }
        
        switch (userInput) {
          case 0:
            redraw=false;
            delay(MENU_UI_POST_DELAY);
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
    
    void ShowInfo() {
      boolean doLoop = true;
      boolean redraw = true;
      int userInput;
      int firstline = 1;
      int lineId = 0;
      long displayRedraw = 0;
    
      while (doLoop) {
        if (redraw) {
          displayRedraw = millis();
          lineId = 0;
          lcd->clear();
          if (firstline == 1) {
            lcd->setCursor(0, lineId);
            lcd->print(F("Hrd Rev: "));
            lcd->print(incSet->getHardware());
            lineId++;
          }
          if (firstline == 1 || firstline == 2) {
            lcd->setCursor(0, lineId);
            lcd->print(F("PI: "));
            lcd->print(incSet->getSerial());
            lineId++;
          }
          if (firstline == 2 || firstline == 3) {
            lcd->setCursor(0, lineId);
            lcd->print(F("rPI IP: "));
            lineId++;
          }
          if (firstline == 3 || firstline == 4) {
            lcd->setCursor(1, lineId);
            lcd->print(incSet->getIP4());
            lineId++;
          }
          if (firstline == 4 || firstline == 5) {
            lcd->setCursor(0, lineId);
            lcd->print(F("Wired MAC: "));
            lineId++;
          }
          if (firstline == 5 || firstline == 6) {
            lcd->setCursor(1, lineId);
            lcd->print(incSet->getWireMAC());
            lineId++;
          }
          if (firstline == 6 || firstline == 7) {
            lcd->setCursor(0, lineId);
            lcd->print(F("Wifi MAC: "));
            lineId++;
          }
          if (firstline == 7 || firstline == 8) {
            lcd->setCursor(1, lineId);
            lcd->print(incSet->getWifiMAC());
            lineId++;
          }
          
          if (firstline == 8 || firstline == 9) {
            lcd->setCursor(0, lineId);
            lcd->print(F("Hardware opts: "));
            lineId++;
          }
          if (firstline == 9 || firstline == 10) {
            String hwSup = F("H ");
            if (incSet->HasCO2Sensor()) {
              hwSup = String(hwSup + F("CO2 "));
            }
            if (incSet->HasO2Sensor()) {
              hwSup = String(hwSup + F("O2 "));
            }
            if (incSet->CountGasRelays() > 0) {
              hwSup = String(hwSup + incSet->CountGasRelays() + F("S "));
            }
            if (incSet->HasLighting()) {
              hwSup = String(hwSup + F("L "));
            }
            if (incSet->HasPiLink()) {
              hwSup = String(hwSup + F("PL "));
            }
            lcd->setCursor(0, lineId);
            lcd->print(hwSup);
            //lcd->print();
            lineId++;
          }
          if (firstline == 10 || firstline == 11) {
            lcd->setCursor(0, lineId);
            lcd->print(F("Software incld: "));
            //lcd->print();
            lineId++;
          }
          if (firstline == 11 || firstline == 12) {
            String swSup = F("H ");
            #ifdef INCLUDE_CO2
            swSup = String(swSup + F("CO2 "));
            #endif
            #ifdef INCLUDE_O2
            swSup = String(swSup + F("O2 "));
            #endif
            #ifdef INCLUDE_LIGHT
            swSup = String(swSup + F("L "));
            #endif
            #ifdef INCLUDE_ETHERNET
            swSup = String(swSup + F("E "));
            #endif
            lcd->setCursor(0, lineId);
            lcd->print(swSup);
            //lcd->print();
            lineId++;
          }
          redraw = false;
        }
        userInput = GetButtonState();
        
        if (userInput != 0) {
          switch (userInput) {
            case 1:
              if (firstline > 1) { firstline--; }
              redraw = true;
              break;
            case 2:
              if (firstline < 11) { firstline++; }
              redraw = true;
              break;
            case 3:
              doLoop=false;
              break;
          }
          delay(MENU_UI_POST_DELAY);
        } else {
          delay(MENU_UI_POST_DELAY);
          if (displayRedraw + 10000 < millis()) {
            firstline++;
            if (firstline > 12) {
              firstline = 1;
            }
          }
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

#define MAINMENU_SET_HEAT 1
#define MAINMENU_SET_CO2 2
#define MAINMENU_SET_O2 3
#define MAINMENU_SET_LITE_ON 4
#define MAINMENU_SET_LITE_OFF 5
#define MAINMENU_PAGE_ADVANCED 6
#define MAINMENU_CONF_HEAT 7
#define MAINMENU_CONF_FAN 8
#define MAINMENU_CONF_C02 9
#define MAINMENU_CONF_O2 10
#define MAINMENU_CONF_LITE 11
#define MAINMENU_PAGE_INFO 12
#define MAINMENU_PAGE_DEFAULTS 13
#define MAINMENU_PAGE_BASIC 14

    int CheckScreenNumber(int screen) {
      if (screen < MAINMENU_SET_HEAT || screen > MAINMENU_PAGE_BASIC) {
        screen = 1; // go to the first screen
      }
      if (screen == MAINMENU_SET_HEAT && incSet->getHeatMode() == 0) {
        // heat disabled, skip heat setpoint screen
        screen++;
      }
      if (screen == MAINMENU_SET_CO2 && incSet->getCO2Mode() < 2) {
        // CO2 maintenance disabled, skip setpoint screen
        screen++;
      }
      if (screen == MAINMENU_SET_O2 && incSet->getO2Mode() < 2) {
        // O2 maintenance disabled, skip setpoint screen
        screen++;
      }
      if (screen == MAINMENU_SET_LITE_ON /*&& incSet->getLightMode() < 1*/) {
        // Lighting system disabled, skip setpoint screen
        screen++;
      }
      if (screen == MAINMENU_SET_LITE_OFF /*&& incSet->getLightMode() < 1*/) {
        // Lighting system disabled, skip setpoint screen
        screen++;
      }
      if (screen == MAINMENU_CONF_C02 && !incSet->HasCO2Sensor()) {
        // CO2 sensor not present, skip configuartion screen
        screen++;
      }
      if (screen == MAINMENU_CONF_O2 && !incSet->HasO2Sensor()) {
        // O2 sensor not present, skip configuartion screen
        screen++;
      }
      if (screen == MAINMENU_CONF_LITE && !incSet->HasLighting()) {
        // Lights not present, skip configuartion screen
        screen++;
      }  
      return screen;
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

        screen = CheckScreenNumber(screen);
        
        switch (screen) {
          case MAINMENU_SET_HEAT:
            if (redraw) {
              DrawMainMenuPage("Set Temp.", "", true, "", "");
              delay(MENU_UI_POST_DELAY);
              redraw=false;
            } else if (userInput == 1) {
              DoVariableAdjust(1);
              redraw = true;
              userInput = 0;
            }
            break;
          case MAINMENU_SET_CO2:
            if (redraw) {
              DrawMainMenuPage("Set CO2", "", true, "", "");
              delay(MENU_UI_POST_DELAY);
              redraw=false;
            } else if (userInput == 1) {
              DoVariableAdjust(2);
              redraw = true;
              userInput = 0;
            }
            break;
          case MAINMENU_SET_O2:
            if (redraw) {
              DrawMainMenuPage("Set O2", "", true, "", "");
              delay(MENU_UI_POST_DELAY);
              redraw=false;
            } else if (userInput == 1) {
              DoVariableAdjust(3);
              redraw = true;
              userInput = 0;
            }
            break;
          case MAINMENU_SET_LITE_ON:
            if (redraw) {
              DrawMainMenuPage("Set LED On", "", true, "", "");
              delay(MENU_UI_POST_DELAY);
              redraw=false;
            } else if (userInput == 1) {
              DoVariableAdjust(4);
              redraw = true;
              userInput = 0;
            }
            break;
          case MAINMENU_SET_LITE_OFF:
            if (redraw) {
              DrawMainMenuPage("Set LED Off", "", true, "", "");
              delay(MENU_UI_POST_DELAY);
              redraw=false;
            } else if (userInput == 1) {
              DoVariableAdjust(5);
              redraw = true;
              userInput = 0;
            }
            break;
          case MAINMENU_PAGE_ADVANCED:
            if (redraw) {
              DrawMainMenuPage("Settings", "", false, "Save", "Advanced");
              redraw=false;
            } else if (userInput == 1) {
              DoSaveSettings();
              doLoop = false;
              userInput = 0;
            }
            break;
          case MAINMENU_CONF_HEAT:
            if (redraw) {
              DrawMainMenuPage("Heating", "", true, "", "");
              delay(MENU_UI_POST_DELAY);
              redraw=false;
            } else if (userInput == 1) {
              DoFeatureToggle(1);
              redraw = true;
              userInput = 0;
            }
            break;
          case MAINMENU_CONF_FAN:
            if (redraw) {
              DrawMainMenuPage("Fan Mode", "", true, "", "");
              delay(MENU_UI_POST_DELAY);
              redraw=false;
            } else if (userInput == 1) {
              DoFeatureToggle(2);
              redraw = true;
              userInput = 0;
            }
            break;
          case MAINMENU_CONF_C02:
            if (redraw) {
              DrawMainMenuPage("CO2", "", true, "", "");
              delay(MENU_UI_POST_DELAY);
              redraw=false;
            } else if (userInput == 1) {
              DoFeatureToggle(3);
              redraw = true;
              userInput = 0;
            }
            break;
          case MAINMENU_CONF_O2:
            if (redraw) {
              DrawMainMenuPage("Oxygen", "", true, "", "");
              delay(MENU_UI_POST_DELAY);
              redraw=false;
            } else if (userInput == 1) {
              DoFeatureToggle(4);
              redraw = true;
              userInput = 0;
            }
            break;
          case MAINMENU_CONF_LITE:
            if (redraw) {
              DrawMainMenuPage("Light", "", true, "", "");
              delay(MENU_UI_POST_DELAY);
              redraw=false;
            } else if (userInput == 1) {
              DoFeatureToggle(5);
              redraw = true;
              userInput = 0;
            }
            break;
          case MAINMENU_PAGE_INFO:
            if (redraw) {
              DrawMainMenuPage("Information", "", true, "", "");
              delay(MENU_UI_POST_DELAY);
              redraw=false;
            } else if (userInput == 1) {
              ShowInfo();
              redraw = true;
              userInput = 0;
            }
            break;  
          case MAINMENU_PAGE_DEFAULTS:
            if (redraw) {
              DrawMainMenuPage("Defaults", "", false, "Reset", "Next");
              redraw=false;
            } else if (userInput == 1) {
              delay(MENU_UI_POST_DELAY);
              incSet->ResetSettingsToDefaults();
              lcd->clear();
              lcd->setCursor(0, 0);
              lcd->print(F("Reset to default"));
              DisplayLoadingBar();
              redraw = true;
              userInput = 0;
            }
            break;
          case MAINMENU_PAGE_BASIC:
            if (redraw) {
              DrawMainMenuPage("Settings", "", false, "Save", "Basic");
              redraw=false;
            } else if (userInput == 1) {
              DoSaveSettings();
              delay(MENU_UI_POST_DELAY);
              doLoop = false;
              userInput = 0;
            }
            break;
        }

        if (!redraw) {
          delay(MENU_UI_POST_DELAY);
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
    }
    
  public:
    void SetupUI() {
      this->lastRefresh = 0;
      this->lcd = new LiquidTWI2(0);
      this->lcd->setMCPType(LTI_TYPE_MCP23017);
      this->lcd->begin(16, 2);
  
      Wire.begin(); // wake up I2C bus
      Wire.beginTransmission(0x20);
      Wire.write(0x00); // IODIRA register
      Wire.write(0xFF); // set all of port A to input
      Wire.endTransmission();
    }

    void AttachSettings(IncuversSettingsHandler* iSettings) {
      this->incSet = iSettings;
    }
  
    void DisplayStartup() {
      this->lcd->setCursor(0,0);
      this->lcd->print(F("Incuvers Model 1"));
      this->lcd->setCursor(0,1);
      this->lcd->print(CentreStringForDisplay(F(SOFTWARE_VER_STRING),16));
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

      #ifdef DEBUG_LIGHT
        longDebugDesc += F("Light, ");
        shortDebugDesc += "L";
      #endif
      
      #ifdef DEBUG_MEMORY
        longDebugDesc += F("Memory, ");
        shortDebugDesc += "M";
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
        case 3 :
          lcd->print(CentreStringForDisplay(F("Default Settings"),16));
          break;
      }
      this->DisplayLoadingBar();
      lcd->clear();
    }
  
    void LCDDrawDefaultUI() {
      if (incSet->getPersonalityCount() >= 3) {
        LCDDrawNewUI();
      }
      if (incSet->getPersonalityCount() < 3) {
        LCDDrawDualLineUI();
      }
    }

    void EnterSetupMode() {
      incSet->MakeSafeState();
      SetupLoop();
      incSet->ReturnFromSafeState();
    }

    void WarnOfMissingHardwareSettings() {
      while (true) {
        this->lcd->clear();
        delay(500);
        this->lcd->setCursor(0,0);
        this->lcd->print(CentreStringForDisplay(F("Hardware not"),16));
        this->lcd->setCursor(0,1);
        this->lcd->print(CentreStringForDisplay(F("initialized!"),16));
        delay(4500);
      }
    }
    
    void DoTick() {
      if ((this->lastRefresh + 1000) < millis()) {
        LCDDrawDefaultUI();
        SerialPrintStatus();
        this->lastRefresh = millis();  
      }
      
      int userInput = GetButtonState();
      if (userInput == 3) {
        EnterSetupMode();  
      }
   
      // Do the alarm orchestrator last as it will reset alarms which we want present in the PrintStatus above.
      // Temporarily diabling the alarm orchestrator to help diagnose intermittent issues.
      //AlarmOrchestrator();
    }
    
};


