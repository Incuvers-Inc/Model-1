#define READSENSOR_TIMEOUT 2500

class IncuversSerialSensor {
  private:
    HardwareSerial* dC;
    String setupString;

  public:
    void Initialize(int pinRx, int pinTx, String modeSet) {
      
      switch (pinRx) {
        case 15:
          this->dC = &Serial3;
          //Serial.println(F("Sensor/serial 3: "));
          break;
        case 17:
          this->dC = &Serial2;
        //  Serial.println(F("Sensor/serial 2: "));
          break;
        case 19:
          this->dC = &Serial1;
      //    Serial.println(F("Sensor/serial 1: "));
          break;
      }
      this->setupString = modeSet;      
      
      this->StartSensor();
    }

    void StartSensor() {
      bool confirmed = false;
      String resp = "";
      char inChar;
      long escapeAfter = millis() + READSENSOR_TIMEOUT;
      
      //Serial.println(F("Starting..."));
      this->dC->begin(9600);
      
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
      //Serial.println("");
    }
    

    String GetDataFromSensor(String req) {
      char inChar;
      String inString = "";            // string to hold incoming data from a serial sensor
      boolean stringComplete = false;    // was all data from sensor received? Check
      boolean dataReadComplete = false;
      int i = 0;
      int j = 0;
      long escapeAfter = millis() + READSENSOR_TIMEOUT;

      while (!dataReadComplete) {
        // request a reading
        if (this->dC->available() > 0) {
          for (int i=0; i < this->dC->available(); i++) {
            this->dC->read();
          }
        }
        this->dC->print(req);
        this->dC->print("\r\n");
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
            if (inChar == '\n' && inString.length() > 5) {
              stringComplete = true;   
              //Serial.print(F("Luminox Sensor Response: "));
               //Serial.println(inString);
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

        if (i >= 5 && i <= 45) {
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

String GetLuminoxSensorDetails() {
    return this->GetDataFromSensor("# 0") + " " + this->GetDataFromSensor("# 1") + " " + this->GetDataFromSensor("# 2");
}

String GetLuminoxSense() {
    return this->GetDataFromSensor("A");
}

String GetCOZIRSensorDetails() {
    return this->GetDataFromSensor("Y");
}
};

