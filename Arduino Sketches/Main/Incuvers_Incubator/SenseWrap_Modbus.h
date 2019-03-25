class IncuversModbusSensor {
  private:
    SoftwareSerial* ss;
    ModbusMaster* node;
    boolean useStreaming;
    int transmitPin;

  public:
    void Initialize(int pinRx, int pinTx, int pinTrans, boolean useStreamMode) {
      this->useStreaming = useStreamMode;
      this->transmitPin = pinTrans;
      pinMode(pinTrans, OUTPUT);
      digitalWrite(pinTrans, 0);
      this->ss = new SoftwareSerial(pinRx, pinTx); // Rx,Tx
    }

    void StartSensor() {
      #ifdef DEBUG_MODBUS
          Serial.print(F("Starting"));
      #endif
      this->ss->begin(115200);
      this->node->begin(1, this->ss);
    }

    void StartListening() {
      #ifdef DEBUG_MODBUS
          Serial.print(F("Listening"));
      #endif
      this->ss->listen();
    }

    String GetSerialSensorReading(int minLen, int maxLen) {
      #ifdef DEBUG_MODBUS
          Serial.print(F("GetSSR("));
          Serial.print(minLen);
          Serial.print(F(","));
          Serial.print(maxLen);
          Serial.println(F(")"));
      #endif
    
      char inChar;
      String lastString = "";                   
      String currString = "";            // string to hold incoming data from a serial sensor
      boolean stringComplete = false;    // was all data from sensor received? Check
      boolean dataReadComplete = false;
      boolean prevReadAvailable = false;
      currString.reserve(maxLen+5); 
      int i = 0;
      int j = 0;
      long escapeAfter = millis() + READSENSOR_MODBUS_TIMEOUT;
      
      return currString;
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
  #ifdef DEBUG_MODBUS
    Serial.print(F("GetDSR('"));
    Serial.print(index);
    Serial.print(F("','"));
    Serial.print(sensorOutput);
    Serial.println(F("')"));
  #endif
  float value = invalidValue;

  
  return value;
}


int GetIntegerSensorReading(char index, String sensorOutput, int invalidValue) {
  #ifdef DEBUG_MODBUS
    Serial.print(F("GetISR('"));
    Serial.print(index);
    Serial.print(F("','"));
    Serial.print(sensorOutput);
    Serial.println(F("')"));
  #endif
  int value = invalidValue;

  
  
  return value;
}


