#define READSENSOR_TIMEOUT 2500

class IncuversSerialSensor {
  private:
#ifndef USE_2560 
    SoftwareSerial* dC;
#else
    HardwareSerial* dC;
#endif
    String setupString;
    String requestString;

  public:
    void Initialize(int pinRx, int pinTx, String modeSet, String reqStr) {
      
#ifndef USE_2560 
      this->dC = new SoftwareSerial(pinRx, pinTx); // Rx,Tx
#else
      switch (pinRx) {
        case 15:
          this->dC = &Serial3;
          #ifdef DEBUG_SERIAL
            Serial.println(F("Serial wrap, Sensor/serial 3: "));
          #endif
          break;
        case 17:
          this->dC = &Serial2;
          #ifdef DEBUG_SERIAL
            Serial.println(F("Serial wrap, Sensor/serial 2: "));
          #endif
          break;
        case 19:
          this->dC = &Serial1;
          #ifdef DEBUG_SERIAL
            Serial.println(F("Serial wrap, Sensor/serial 1: "));
          #endif
          break;
      }
#endif
      this->setupString = modeSet;      
      this->requestString = reqStr;

      //this->StartSensor();
    }

    void StartSensor() {
      bool confirmed = false;
      String resp = "";
      char inChar;
      long escapeAfter = millis() + READSENSOR_TIMEOUT;
      
      #ifdef DEBUG_SERIAL
        Serial.println(F("Starting..."));
      #endif
      this->dC->begin(9600);
      
      #ifdef DEBUG_SERIAL
        Serial.print(F("InitISS - modeSet: "));
        Serial.print(this->setupString);
        Serial.print(F(", result: "));
      #endif
      this->dC->print(this->setupString);
      this->dC->print("\r\n");
      
      while(!confirmed && escapeAfter > millis()) {
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
      #ifdef DEBUG_SERIAL
        Serial.println("");
      #endif
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
        // request a reading
        this->dC->print(this->requestString);
        this->dC->print("\r\n");
        #ifdef DEBUG_SERIAL
          Serial.print(F("Sending Request: "));
          Serial.println(this->requestString);
        #endif
        delay(200); // there will likely be a 100ms delay on the response
        i=0;
        inString = "";
        stringComplete = false;
         
        while(!stringComplete) {
          if (this->dC->available() > 0) {
            inChar = (char)this->dC->read();
            if (inChar != '\n' && inChar != '\r' ) {    
              inString += inChar;
              i++;
            }
            if (inChar == '\n' && inString.length() > minLen) {
              stringComplete = true;   
              #ifdef DEBUG_SERIAL
                Serial.print(F("\tCompleted String: "));
                Serial.println(inString);
              #endif
            }
          }

          if (escapeAfter < millis()) {
            #ifdef DEBUG_SERIAL
              Serial.println(F("\tEscaping the try due to timeout"));
            #endif
            // fake the end of the string as we hit the timeout
            stringComplete = true;
          }
        }

        if (i >= minLen && i <= maxLen) {
          dataReadComplete = true;

          #ifdef DEBUG_SERIAL
            Serial.print(F("Data read complete: "));
            Serial.println(i);
          #endif
        } else {
          #ifdef DEBUG_SERIAL
            Serial.print(F("Data read incomplete with i: "));
            Serial.print(i);
            Serial.print(F(", string: "));
            Serial.print(inString);
          #endif
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

      // clear the queue, just in case
     /* #ifdef DEBUG_SERIAL
        Serial.print(this->dC->available());
        Serial.println(F(" bytes in queue to be cleared."));
      #endif
      while(this->dC->available()) {
        inChar = (char)this->dC->read();
      }*/

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


