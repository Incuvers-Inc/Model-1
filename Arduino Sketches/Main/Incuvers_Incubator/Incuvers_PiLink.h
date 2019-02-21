#ifdef USE_2560
/*
 * Incuvers PiLink code.
 * 
 * Only functional on 1.0.0+ control board units.
 * Requires the CRC32 library by Christopher Baker.
 */

#define MAX_INPUT_MESSAGE_SIZE 92
#define MAX_PAYLOAD_SIZE 80

class IncuversPiLink {
  private:
    IncuversSettingsHandler* incSet;
    bool isEnabled;

    // Each command will be given on a single line.  In the format of Len*CRC32$Param|Value&Param|Value
    // A command line may not be longer than 92 characters.
    void CheckForCommands() {
      if (PILINK_SERIALHANDLE.available()) {
        boolean lineComplete = false;
        int strLen = 0;
        char stringRead[MAX_INPUT_MESSAGE_SIZE + 1] = "";

        while (!lineComplete && strLen < MAX_INPUT_MESSAGE_SIZE) {
          if (PILINK_SERIALHANDLE.available()) {
            char c = PILINK_SERIALHANDLE.read();
            
            if (c == '\n') {
              lineComplete = true;
            } else {
              stringRead[strLen] = c;
              strLen++;
            }
          } else {
            delay (42); // wait for buffer to get more data
          }
        }

        stringRead[strLen] = 0;
        
        #ifdef DEBUG_PILINK
          Serial.print(F("Read a line from Pilink: "));
          Serial.println(stringRead);
        #endif

        char* msgLenText = strtok(stringRead, "*");
        int msgLen =  atoi(msgLenText);
        
        if (msgLen > 0 && msgLen <= MAX_PAYLOAD_SIZE && strLen == (msgLen + strlen(msgLenText) + 10)) {
          char* includedCRC = strtok(NULL, "$");
          
          if (CheckCRCOnMessage(includedCRC, stringRead))
          {
            boolean processingComplete = false;
            while (!processingComplete) {
              char* param = strtok(NULL, "|");  
              char* valueText = strtok(NULL, "&");
              int value = 0;
              if (valueText != NULL) {
                value = atoi(valueText);
              }

              if (param != NULL) {
                 if(strcmp(param, "TP") == 0){
                    float newTemp = (float)value * 0.01;
                    if (newTemp > TEMPERATURE_MIN && newTemp < TEMPERATURE_MAX) {
                      this->incSet->setTemperatureSetPoint(newTemp);
                    } else {
                      #ifdef DEBUG_PILINK
                        Serial.println(F("Requested temperature set point is outside of min/max"));
                      #endif
                    }
                 } else if(strcmp(param, "CP") == 0) {
                    float newCO2 = (float)value * 0.01;
                    if (newCO2 > CO2_MIN && newCO2 < CO2_MAX) {
                      this->incSet->setCO2SetPoint(newCO2);
                    } else {
                      #ifdef DEBUG_PILINK
                        Serial.println(F("Requested CO2 set point is outside of min/max"));
                      #endif
                    }
                 } else if(strcmp(param, "OP") == 0) {
                    float newO2 = (float)value * 0.01;
                    if (newO2 > OO_MIN && newO2 < OO_MAX) {
                      this->incSet->setO2SetPoint(newO2);
                    } else {
                      #ifdef DEBUG_PILINK
                        Serial.println(F("Requested O2 set point is outside of min/max"));
                      #endif
                    }
                }
              } else {
                processingComplete = true;
              }
            }
            
            
          } else {
            // message failed CRC check
            #ifdef DEBUG_PILINK
              Serial.println(F("Message failed CRC32 check."));
            #endif
          }   
        } else {
          // message failed pre-checks
          #ifdef DEBUG_PILINK
            Serial.println(F("Message failed pre-CRC32 checks."));
            Serial.print(F("msgLen"));
            Serial.print(msgLen);
            Serial.print(F(" strLen"));
            Serial.print(strLen);
            Serial.print(F(" msgLenText"));
            Serial.println(strlen(msgLenText));

            //msgLen > 0 && msgLen <= MAX_PAYLOAD_SIZE && strLen == (msgLen + strlen(msgLenText)
          #endif
        }
        
        
      }
    }

    bool CheckCRCOnMessage(char* oldCRC, char* msg){
      CRC32 crc;
        
      for (int i = 0; i < strlen(msg); i++) {
        crc.update(msg[i]);
      }
      String tmpCRC = String(crc.finalize(), HEX);
      char newCRC[9] = "";
      tmpCRC.toCharArray(newCRC, 8);
      
      #ifdef DEBUG_PILINK
        Serial.print(F("Old CRC: "));
        Serial.print(oldCRC);
        Serial.print(F(" New CRC: "));
        Serial.print(newCRC);
      #endif

      if (strcmp(oldCRC, newCRC) == 0) {
        #ifdef DEBUG_PILINK
          Serial.println(F(" Check!"));
        #endif  
        return true;
      } else {
        #ifdef DEBUG_PILINK
          Serial.println(F(" Failed!"));
        #endif  
        return false;
      }
    }
    
    void SendStatus() {
      PILINK_SERIALHANDLE.println(incSet->GenerateStatusLine(true));
    }
    
  public:
    void SetupPiLink(IncuversSettingsHandler* iSettings) {
      this->incSet = iSettings;
      if (this->incSet->HasPiLink()) {
        #ifdef SERIALPILINKSETTINGS
          PILINK_SERIALHANDLE.begin(SERIALPILINKSETTINGS);
        #endif
      }
    }

    void DoTick() {
      if (this->incSet->HasPiLink()) {
        CheckForCommands();
        SendStatus();
      }
    }

};

#else
class IncuversPiLink {
  public:
    void SetupPiLink(IncuversSettingsHandler* iSettings) {
    }

    void DoTick() {
    }

};
#endif
