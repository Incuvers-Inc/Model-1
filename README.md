
# Incuvers Environment Control
This code is intended for the Incuvers telemetric chamber [http://www.incuvers.com](http://www.incuvers.com).
The Arduino code manages the incubator environment and physical user interface via buttons on the unit.
A Raspberry Pi connects to the Arduino control board and processes the telemetry to the cloud.

## Arduino

### Environment control

#### Gas
The CO2 and O2 work with the same principals and are grouped as "gas".
Not all units are equipped with gas controls.

##### Modes
Monitor and maintain for both CO2 and O2. The modes can be changed with the physical interface.
They both can be one of three possible values: `off`, `read` and `maintain`.
In `off` mode, all controls, and monitoring and alarms are deactivated.
In `read` mode, controls and alarms are deactivated but the sensor output can be used to monitor the status.
In `maintain` mode, the system will actively try to reach a target set point and the sensor output is monitored.
If the target is not reached an alarm may be raised (see Alarm below)

For software setting and monitoring, the keys for CO2 and O2 modes are `CM` and `OM` respectively.
They both can have one of three possible values:
`0 = off`, `1 = read` and `2 = maintain`.


##### Alarms
An alarm is raised when the system detects an anomalous state or difficulty in reaching targets.
Alarms are usually activated when a subsystem is in `maintain` mode.
Thus it is important not to activate `maintain` mode for a particular subsystem if the incubator does not support it.
Failure to do so can result in an alarm which will turn off the unit.

The alarm for the CO2 and O2 subsystems have key `CA` and `OA` respectively. They can have the value of `0 = off`, `1 = report`, `2 = alarm`.

#### Temperature
There are two heating units with corresponding sensors that keep the chamber at a specified temperature.
A fan, to aid in circulation, is built-in next to the principal heating pad.

There is a secondary heating pad placed on the door to prevent condensation.
There is also a door thermometer placed next to it to prevent overheating.

The chamber and door temperature sensors have keys `TD` and `TC` respectively.
There is only one target temperature set point that is used as a target to both heating pads, it has the key `TP`.
The target temperature can be changed from the physical interface.

##### Modes
The heating system has a Monitor and maintain mode. The fan has four different running modes.

The heating subsystem has two modes on or off.
These modes can be changed on the physical interface or via software by assigning the key `TM` to one of two integer values `0 = off` or `1 = on`.

There are four fan modes that are held in the key `FM` :
`0 = off`,
`1 = on during heat + 30 seconds after`,
`2 = on during heat + 60 seconds`,
`3 = on during heat + 50% of time`,
`4 = always on`.
By default the fan is always on and cannot be controlled form the physical interface.



##### Alarms
An alarm is raised when the system detects an anomalous state or difficulty in reaching targets.
The alarm for the heating subsystem has key `TA`, and can have the value of `0 = off`, `1 = report`, `2 = alarm`

### Physical user interface
Red Buttons: single press, double press.
Make sure to save the settings, or else they will be reset after a reboot.

## Monitor
The Arduino control board broadcasts sensor readings, target, alarms and modes over serial.
The Monitor module takes care of processing the serial messages, as well as providing status updates to the Arduino.


## Message structure
PiLink will be a simple serial connection.
At the end of each loop run the Arduino will send a string to the Pi giving the current status.
A CRC checksum is included to uncover any lines which are corrupt.
Corrupt lines will be ignored as the next should be transmitted within the next 1-5 seconds and so retransmitting the previous line would be a waste of resources.
When the Pi wishes to update the settings or configuration of the Arduino, one or more commands will be sent over the PiLink.
The Arduino won’t implicitly confirm the changes however the Pi should verify that the changes have been accepted by ensuring that the settings reported back in the next status update reflect the changes.
Wherever possible the Parameter names will match in the Pi &rarr; Arduino and Arduino &rarr; Pi systems.
A command line coming from the Pi will not include a payload of more than 52 characters making a command no longer than 64 characters long (this limit is due to the default serial buffer size on the Arduino.)

### Special characters
There are special characters used parse the message and cannot be used as part of the message: the ampersand `&`; pipe `|`; tilde `~` and dollar sign `$`.



### RPi &rarr; Arduino
The message has the following format:
`Len*CRC32$Param|Value&Param|Value`
where `Len` is the length of the message payload,
`CRC32` is a checksum of the payload contents,
and each `Param|Value` pair include a unique parameter Id and a positive Integer value (decimal numbers will be converted on the Arduino.)

Messages will always contain the `Len` block, the `CRC32` block and one or more `Param|Value` tuples.
The `Len` block is always the first to lead and is terminated by the special character `~`.
What follows is the checksum `CRC32` which is used to verify the message integrity.
The `CRC32` is terminated by the special character `$`.

Every `Param|Value` pairs are separated by the `&` character. The `Param` and `Value` within the tuple is separated by the `|` character.

In the case of a corrupted message, that does not contain the special parsing characters, the Arduino drops the message after trying to interpret 80 characters.

Examples:  

`30~xxxxxxxx$IPA|192&IPB|168&IPC|42&IPD|142`  
In this first example, the Pi is providing its configured network address to the Arduino for display in the UI.

In another example:
`20~xxxxxxxx$TP|3750&CP|1950&LS|1`

the Pi is directing the Arduino to change the temperature set point (with key `TP`) to 37.5, the CO2 set point (with key `CP`) to 19.50 and to turn on the lighting system (with key `LS`).

The following table describes the possible keys:

Administrative:

 | Param      |Description          | Unit 	    | Example |
 | ----       | ----                | ----	    | ----	  |
 |`ID`        |Serial Identifier    | ASCII string  | ...     |
 |`IP4`       |Pi active IPv4 address | ASCII string | ...     |
 |`MWR`       |Pi wired MAC address | ASCII string | ...     |
 |`MWF`       |Pi wifi MAC address  | ASCII string | ...     |
 |`SS`        |Save settings        |Boolean    | `1` (save)     |
 |`CG`        |CO2 calibration      | Boolean       |`1` (true)


Environmental:

 | Param |Description          | Unit 	               | Example 	|
 | ----  | ----                | ----	                 | ----	|
 |`FM`   |Fan mode             | Enum                  | `0` (off)   |
 |`TM`   |Heating Mode         | Enum                  | `1` (on)   |
 |`TP`   |Temperature set point| Hundredths of degree C|`3700` (37.00C) |
 |`CM`   |CO2 Mode             | Enum                  | `1` (read) |
 |`CP`   |CO2 set point        | Hundredths of %       |`520` (5.20%)
 |`OM`   |O2 mode              | Enum                  | `2`  (maintain) |
 |`OP`   |O2 set point         | Hundredths of %       |`2000` (20.00%)

#### Gas sensor calibration
 The needs to be calibrated once in a while.
 Triggering the calibration procedure through the UI isn't yet supported (a manual call to `iCO2->CalibrateFreshAir()` is needed).
The calibration is can be triggered by sending the string the parameter pair `CG|1` from a connected raspberry pi (via the message `4~3ab55f3b$CG|1`).

The arduino will confirm the request by advertising a recent calibration by broadcasting `CG|1` instead of `CG|0`.
After 5 days or a reset, the default broadcast of `CG|0` resume.

The O2 sensor does not need periodic calibration.

##### Fresh-air calibration procedure.
Prior to a calibration call, the environment in the incubator must be as close as possible to "fresh-air" conditions (400ppm is assumed).
Open the door of the chamber with heating turned off and the fans turned on.
The CO2 and O2 controls must both be set to `monitor`).
Adequate time must have elapsed (at least 5 minutes) for the interior of the chamber to equilibrate.
Shut the door and make a system call to `CalibrateFreshAir()` by sending `CG|1`.
The CO2 sensor is now calibrated.
