#define READSENSOR_TIMEOUT 1500

class IncuversSerialSensor {
  private:
#ifndef USE_2560 
    SoftwareSerial* dC;
#else
    HardwareSerial* dC;
#endif
    String requestString;

  public:
    void Initialize(int pinRx, int pinTx, String modeSet, String reqStr) {
      bool confirmed = false;
      String resp = "";
      char inChar;
      
#ifndef USE_2560 
      this->dC = new SoftwareSerial(pinRx, pinTx); // Rx,Tx
#else
      switch (pinRx) {
        case 15:
          this->dC = &Serial3;
          break;
        case 17:
          this->dC = &Serial2;
          break;
        case 19:
          this->dC = &Serial1;
          break;
      }
#endif      

      #ifdef DEBUG_SERIAL
        Serial.print(F("InitISS - modeSet, result: "));
      #endif
      this->dC->println(modeSet);
      while(!confirmed) {
        delay(250);
        if (this->dC->available() > 0) {
           inChar = (char)this->dC->read();
           if (inChar != '\n' && inChar != '\r' ) {    
             resp += inChar;
           }
           if (inChar == '\n') {
            // We assume that the response is correct
            #ifdef DEBUG_SERIAL
              Serial.print(resp);
            #endif
            confirmed = true;   
           }
        }
      }

      this->requestString = reqStr;
    }

    void StartSensor() {
      #ifdef DEBUG_SERIAL
          Serial.print(F("Starting"));
      #endif
      this->dC->begin(9600);
    }

    String GetSerialSensorReading(int minLen, int maxLen) {
      #ifdef DEBUG_SERIAL
          Serial.print(F("GetSSR("));
          Serial.print(minLen);
          Serial.print(F(","));
          Serial.print(maxLen);
          Serial.println(F(")"));
      #endif
    
      char inChar;
      String inString = "";            // string to hold incoming data from a serial sensor
      boolean stringComplete = false;    // was all data from sensor received? Check
      boolean dataReadComplete = false;
      int i = 0;
      int j = 0;
      long escapeAfter = millis() + READSENSOR_TIMEOUT;

      #ifndef USE_2560 
        // We don't have a hardware serial interface, so make our software serial interface active.
        this->dC->listen();
      #endif
      
      while (!dataReadComplete) {
        #ifdef DEBUG_SERIAL
          if (this->dC->available() > 0) {
            Serial.print(this->dC->available());
            Serial.println(F(" b avail prior to req, clearing Q"));
          }
        #endif

        // clear the queue
        while(this->dC->available()) {
          inChar = (char)this->dC->read();
          if (inChar != '\n' && inChar != '\r' ) {    
            inString += inChar;
          }
          if (inChar == '\n') {
            #ifdef DEBUG_SERIAL
              Serial.print(F("Q: "));
              Serial.println(inString);
            #endif
            inString = "";
           }
        }

        // request a reading
        this->dC->println(this->requestString);
        while(!stringComplete) {
          if (this->dC->available() > 0) {
            inChar = (char)this->dC->read();
            if (inChar != '\n' && inChar != '\r' ) {    
              inString += inChar;
              i++;
            }
            if (inChar == '\n') {
              stringComplete = true;   
            }
          }
        }

        if (i >= minLen && i <= maxLen) {
          dataReadComplete = true;
        }

        j++;
        
        if(escapeAfter < millis()){
          #ifdef DEBUG_SERIAL
            Serial.print(F("\tEscaping after "));
            Serial.print(j);
            Serial.println(F(" loops"));
          #endif
          dataReadComplete=true;
        }
    
      }
      #ifdef DEBUG_SERIAL
        Serial.print(F("\tReturning: "));
        Serial.println(inString);
      #endif
      return inString;
    }
};

boolean IsNumeric(String string) {
  boolean isNum = true;
  for (int i = 0; i < string.length(); i++) {
    if (!(isDigit(string.charAt(i)) || string.charAt(i) == '+' || string.charAt(i) == '-' || string.charAt(i) == '.')) {
      isNum = false;
    }
  }
  return isNum;
}


float GetDecimalSensorReading(char index, String sensorOutput, float invalidValue) {
  #ifdef DEBUG_SERIAL
    Serial.print(F("GetDSR('"));
    Serial.print(index);
    Serial.print(F("','"));
    Serial.print(sensorOutput);
    Serial.println(F("')"));
  #endif
  float value = invalidValue;

  int iIndex = sensorOutput.lastIndexOf(index);
  String shortenedString = sensorOutput.substring(iIndex+2, sensorOutput.length());
  #ifdef DEBUG_SERIAL
    Serial.print(F("\tShort: "));
    Serial.println(shortenedString);
  #endif
  int iSpace = shortenedString.indexOf(" ");
  String readingString = shortenedString.substring(0, iSpace);
  readingString.trim();
  #ifdef DEBUG_SERIAL
    Serial.print(F("\tReading: "));
    Serial.println(readingString);
  #endif
  if (IsNumeric(readingString)) {
    value = readingString.toFloat();
  }

  #ifdef DEBUG_SERIAL
    Serial.print(F("\tValue: "));
    Serial.println(value);
  #endif
  return value;
}


int GetIntegerSensorReading(char index, String sensorOutput, int invalidValue) {
  #ifdef DEBUG_SERIAL
    Serial.print(F("GetISR('"));
    Serial.print(index);
    Serial.print(F("','"));
    Serial.print(sensorOutput);
    Serial.println(F("')"));
  #endif
  int value = invalidValue;

  int iIndex = sensorOutput.lastIndexOf(index);
  String shortenedString = sensorOutput.substring(iIndex+2, sensorOutput.length());
  #ifdef DEBUG_SERIAL
    Serial.print(F("\tShort: "));
    Serial.println(shortenedString);
  #endif
  int iSpace = shortenedString.indexOf(" ");
  String readingString = shortenedString.substring(0, iSpace);
  readingString.trim();
  #ifdef DEBUG_SERIAL
    Serial.print(F("\tReading: "));
    Serial.println(readingString);
  #endif
  if (IsNumeric(readingString)) {
    value = readingString.toInt();
  }

  #ifdef DEBUG_SERIAL
    Serial.print(F("\tValue: "));
    Serial.println(value);
  #endif
  
  return value;
}


