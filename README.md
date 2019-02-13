
# Incuvers Environment Control
This code is intended for the Incuvers telemetric chamber [http://www.incuvers.com](http://www.incuvers.com).
The Arduino code manages the incubator environmental and physical user interface via buttons on the unit.
A Raspberry Pi connects to the Arduino control board and processes the telemetry to the cloud.

## Arduino

### Environment control

#### Gas
The CO2 and O2 work with the same principals and are grouped as "gas".
Not all units are equipped with gas controls.

##### Modes
Monitor and maintain for both CO2 and O2

##### Alarms
An alarm is raised when the system detects an anomalous state or difficulty in reaching targets.

#### Temperature
There are two heating units with corresponding sensors that keep the chamber at a specified temperature.
A fan, to aid in circulation, is built-in next to the principal heating pad.
##### Modes
The heating system has a Monitor and maintain mode. The fan has four different running modes.

##### Alarms
An alarm is raised when the system detects an anomalous state or difficulty in reaching targets.

### Physical user interface
Buttons

## Monitor
The Arduino control board broadcasts sensor readings, target, alarms and modes over serial.
The Monitor module takes care of processing the serial messages, as well as providing status updates to the Arduino.


## Message structure
Describe serial message structure.

#### Special characters
