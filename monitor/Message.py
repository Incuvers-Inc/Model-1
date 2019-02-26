import base64
import binascii
import json
import uuid
import serial
import socket
import struct
import sys
import threading
import time
import fcntl


"""
.. module:: Message
    :platform: Raspbian
    :synopsis: Incubator module for serial communication between RPi and Arduino

"""
class ArduinoDirectiveHandler():
    """ Class that handles sending commands to the arduino and verifying
    that the csommands have been accepted and acted uopn by the control
    board.
    """
    def __intit__(self):
        ''' Initialization
        Args:
          None
        '''
        self.queuedictionary = {}

    def enqueue_parameter_update(self, param, value):
        """ Adds a parameter to the update queue, the updates will be sent
        at a later time.  If this update follows another requested update, 
        overwrite it.  The item will remain in the queue until the action
        has been verified
        Args:
          param (str): the identifier of the parameter to update
          value (int): the value of the parameter to update
        """
        if param in queuedictionary:
            print("The " + param +
                  " parameter was already in the dictionary, updating.")
        queuedictionary[param] = value

    def get_arduino_command_string(self):
        """ Returns a complete string to send to the arduino in order to
        update the running parameters.  Commands can be no longer than 80 
        characters and won't involve removing the command from the queue,
        a separate system will verify that the change has been made.
        Args:
          None
        """
        arduino_command_string = ""
        for param in queuedictionary:
            if (len(arduino_command_string) + len(param) + len(str(queuedictionary[param])) + 2) <= 80:
                if len(arduino_command_string) > 0:
                    arduino_command_string = arduino_command_string + "&"
                arduino_command_string = arduino_command_string + param + "|" + str(queuedictionary[param])
        commandLen = len(arduino_command_string)
        calcCRC = binascii.crc32(arduino_command_string.encode())
        arduino_command_string = str(commandLen) + "~" + format(calcCRC, 'X') + "$" + arduino_command_string

        return arduino_command_string

    def verify_arduino_response(self, last_sensor_frame):
        """ Will check all commands in the queuedictionary and pop off any
        for which the state coming from the arduino matches the desired level
        Args:
          last_sensor_frame (dict): the last sensorframe received from the Arduino
        """

    def clear_queue(self):
        """ Command to erase everything in the queue.
        Args:
          None
        """
        for param in queuedictionary:
            queuedictionary.pop(param)

    def get_arduino_mac_update_command_string(self, param="ME", mac_addr="00:00:00:00:00:00"):
        """ Should only be called shortly after a boot
        Args:
          param (str): two-letter code for the start of the parameter (ME for Ethernet, ML for Wifi)
          mac_addr (str): colon-separated string of the current system's MAC address
        """
        self.clear_queue()
        mac_bits = mac_addr.split(":")
        self.enqueue_parameter_update(param + "A", mac_bits[0])
        self.enqueue_parameter_update(param + "B", mac_bits[1])
        self.enqueue_parameter_update(param + "C", mac_bits[2])
        self.enqueue_parameter_update(param + "D", mac_bits[3])
        self.enqueue_parameter_update(param + "E", mac_bits[4])
        self.enqueue_parameter_update(param + "F", mac_bits[5])
        cmd_string = self.get_arduino_command_string()
        self.clear_queue()
        return cmd_string

    def get_arduino_ip_update_command_string(self, ip_addr):
        """ Should only be called after a boot or when noticing an IP
        change
        Args:
          ip_addr (str): string containing the IP address of the item
        """
        self.clear_queue()
        ip_bits = ip_addr.split(".")
        self.enqueue_parameter_update("IPA", ip_bits[0])
        self.enqueue_parameter_update("IPB", ip_bits[1])
        self.enqueue_parameter_update("IPC", ip_bits[2])
        self.enqueue_parameter_update("IPD", ip_bits[3])
        cmd_string = self.get_arduino_command_string()
        self.clear_queue()
        return cmd_string
    

class Interface():
    """ Class that holds everything important concerning communication.

    Mainly stuff about the serial connection, but also
    identification and internet connectivity. Objects indended
    to be displayed on the Incubator LCD should be found in this class.

    Attributes:
        serial_connection (:obj:`Serial`): serial connection from GPIO
    """
    def __init__(self):
      ''' Initialization
      Args:
        None
      '''
      self.serial_connection = serial.Serial('/dev/serial0', 9600)

    def connects_to_internet(self, host="8.8.8.8", port=53, timeout=3):
      """ Tests internet connectivity
      Test intenet connectivity by checking with Google
      Host: 8.8.8.8 (google-public-dns-a.google.com)
      OpenPort: 53/tcp
      Service: domain (DNS/TCP)
      Args:
        host (str): the host to test connection
        port (int): the port to use
        timeout (timeout): the timeout value (in seconds)
      """

      try:
        socket.setdefaulttimeout(timeout)
        socket.socket(socket.AF_INET, socket.SOCK_STREAM).connect((host, port))
        return True
      except Exception as err:
        print(err.message)
        return False

    def get_serial_number(self):
      """ Get unique serial number
      Get a unique serial number from the cpu
      Args:
        None
      """
      serial = "0000000000"
      try:
        with open('/proc/cpuinfo','r') as fp:
          for line in fp:
            if line[0:6]=='Serial':
              serial = line[10:26]
        if serial == "0000000000":
          raise TypeError('Could not extract serial from /proc/cpuinfo')
      except FileExistsError as err :
        serial = "0000000000"
        print(err.message)

      except TypeError as err :
        serial = "0000000000"
        print(err.message)
      return serial

    def get_mac_address(self):
      """ Get the ethernet MAC address
      This returns in the MAC address in the canonical human-reable form
      Args:
        None
      """
      hex = uuid.getnode()
      formatted = ':'.join(['{:02x}'.format((hex >> ele) & 0xff) for ele in range(0,8*6,8)][::-1])
      return formatted

class Sensors():
    """ Class to handle sensor communication
    This
    Attributes:
        sensorframe (dict : Dictionary holding all sensor key-value pairs
        verbosity (int) : set sthe amount of information printed to screen
        arduino_link (): Instance of the Interface() class
        lock (): Instance of threading Lock to
        monitor (): Process that continuously read incomming serial messages
    """
    def __init__(self):
        """ Initialization
        """
        # dictionary to hold incubator status
        self.sensorframe = {}

        # set verbosity=1 to view more
        self.verbosity = 1

        self.arduino_link = Interface()

        self.lock = threading.Lock()
        self.monitor = threading.Thread(target=self.monitor_incubator_message)
        self.monitor.setDaemon(True)
        self.monitor.start()

    def __str__(self):
        with self.lock:
            toReturn = self.sensorframe
        return str(toReturn)

    def json_sensorframe(self):
        """ JSON object containing arduino serial message
        Returns a JSON formated string of the current state
        Args:
            None
        """
        with self.lock:
            toReturn = {'incubator': self.sensorframe}
        return toReturn

    def monitor_incubator_message(self):
        ''' Processes serial messages from Arduino
        This runs continuously to maintain an updated
        status of the sensors from the arduino
        '''
        if (self.verbosity == 1):
            print("starting monitor")
        while True:
            try:
                line = self.arduino_link.serial_connection.readline()
                if self.checksum_passed(line):
                    if (self.verbosity == 1):
                        print("checksum passed")
                    with self.lock:
                        self.save_message_dict(line)
                else:
                    if (self.verbosity == 1):
                        print('Ign: ' + line.decode().rstrip())
            except BaseException:
                time.sleep(1)
                print('Error: ', sys.exc_info()[0])
                # return

    def checksum_passed(self, string):
        ''' Check if the checksum passed
        recompute message checksum and compares with appended hash
        Args:
            string (str): a string containing the message, having the following
                          format: Len*CRC32$Param|Value&Param|Value

        '''


        if string.decode().len()>92:
            if (self.verbosity == 1):
                print("Length Fail: received string is too long")
            return False

        lineSplit = string.decode().split('~')
        if len(lineSplit) != 2:
            if (self.verbosity == 1):
                print("Corrupt message: while splitting with length special character '*' ")
            return False

        # extract the Len
        msg_len=lineSplit[0]
        lineSplit = lineSplit[1].decode().split('$')
        if len(lineSplit) != 2:
            if (self.verbosity == 1):
                print("Corrupt message: while splitting with length special character '$' ")
            return False
        # extract the CRC
        msg_crc=lineSplit[0]
        calcCRC = binascii.crc32(lineSplit[1].encode().rstrip())
        if format(calcCRC, 'X') == lineSplit[0]:
            if (self.verbosity == 1):
                print("CRC32 matches")
            return True
        else:
            if (self.verbosity == 1):
                print(
                    "CRC32 Fail: calculated " +
                    format(calcCRC,'X') +
                    " but received " +
                    lineSplit[0])
            return False


    def save_message_dict(self, msg):
        '''
        Takes the serial output from the incubator and creates a dictionary
        using the two letter ident as a key and the value as a value.
        Args:
            msg (string): The raw serial message (including the trailing checksum)
        Only use a message that passed the checksum!
        NOTE: The time stamp from the arduino is replaced here
        '''

        msg_serial = msg.decode().split('$')[1]
        self.sensorframe = {}
        for params in msg_serial.split('&'):
            kvp=params.split("|")
            if len(kvp) != 2:
                print("ERROR: bad key-value pair")
            else:
                self.sensorframe[kvp[0]] = kvp[1]
        self.sensorframe['time'] = time.strftime(
            "%Y-%m-%d %H:%M:%S", time.gmtime())
        return


if __name__ == '__main__':

    test_connections = False
    if test_connections:
        iface = Interface()
        print("Hardware serial number: {}".format(iface.get_serial_number()))
        print("Hardware address (ethernet): {}".format(iface.get_mac_address()))
        print("Can connect to the internet: {}".format(iface.connects_to_internet()))
        print("Can contact the mothership: {}".format(iface.connects_to_internet(host='35.183.143.177', port=80)))
        print("Has serial connection with Arduino: {}".format(iface.serial_connection.is_open))
        del iface

    test_commandset = True
    if test_commandset:
        arduino_handler = ArduinoDirectiveHandler()


        del arduino_handler

    monitor_serial = False
    if monitor_serial:
        mon = Sensors()
        mon.verbosity = 1
        print("Has serial connection with Arduino: {}".format(mon.arduino_link.serial_connection.is_open))
        print("Displaying serial data")
        while True:
            time.sleep(5)
            print(mon)
        del mon
