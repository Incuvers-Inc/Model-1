
class IncuversTempSensor {
  private:
    OneWire* oneWire;
    DallasTemperature* tempSensors;

  public:
    void Initialize(int pin) {
      this->oneWire = new OneWire(pin);       // Setup a oneWire instance to communicate with ANY OneWire devices
      this->tempSensors = new DallasTemperature(this->oneWire);          // Pass our oneWire reference to Dallas Temperature.

      this->tempSensors->begin();
    }

    String GetSensorIdent() {
      int i, j, k;
      byte addr[8];
      String addresses;

      addresses = "";
      j=0;
      while(oneWire.search(addr)) {
        if (addresses != "") {
          addresses = addresses + "|"
        }
        for( i = 0; i < 8; i++) {
          addresses = addresses + String(addr[i]);
          if (i<7) {
             addresses = addresses + ":"
          }
        }
        j++;
      }


  Serial.print("Found ");
  Serial.print(j);
  Serial.println(F(" OneWire devices"));

      return addresses;
    }
};

