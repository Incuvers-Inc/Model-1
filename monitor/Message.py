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
import commands

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

    def __init__(self):
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

        if param in self.queuedictionary:
            print("The " + param +
                  " parameter was already in the dictionary, updating.")
        self.queuedictionary[param] = value

    def get_arduino_command_string(self):
        """ Returns a complete string to send to the arduino in order to
        update the running parameters.  Commands can be no longer than 80
        characters and won't involve removing the command from the queue,
        a separate system will verify that the change has been made.

        Args:
          None
        """
        arduino_command_string = ""
        for param in self.queuedictionary:
            if (len(arduino_command_string) + len(param) + len(str(self.queuedictionary[param])) + 2) <= 80:
                if len(arduino_command_string) > 0:
                    arduino_command_string = arduino_command_string + "&"
                arduino_command_string = arduino_command_string + \
                    param + "|" + str(self.queuedictionary[param])
        commandLen = len(arduino_command_string)
        arduino_command_string = arduino_command_string + "\r"
        calcCRC = binascii.crc32(arduino_command_string.encode()) & 0xFFFFFFFF
        
        arduino_command_string = str(commandLen) + "~" + format(calcCRC,
                                                                'x') + "$" + arduino_command_string + "\n"

        return arduino_command_string

    def verify_arduino_response(self, last_sensor_frame):
        """ Will check all commands in the queuedictionary and pop off any
        for which the state coming from the arduino matches the desired level

        Args:
          last_sensor_frame (dict): the last sensorframe received from the Arduino
        """

        return()

    def clear_queue(self):
        """
        Command to erase everything in the queue.

        Args:
            None
        """

        self.queuedictionary.clear()

    def get_arduino_mac_update_command_string(self, param="MWR", mac_addr="00:00:00:00:00:00"):
        """ Should only be called shortly after a boot

        Args:
          param (str): three-letter code for the start of the parameter (MWR for Ethernet, MWF for Wifi)
          mac_addr (str): colon-separated string of the current system's MAC address
        """
        self.clear_queue()
        self.enqueue_parameter_update(param, "".join(mac_addr.split(":")))
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
        self.enqueue_parameter_update("IP4", ip_addr)
        cmd_string = self.get_arduino_command_string()
        self.clear_queue()
        return cmd_string

    def get_arduino_serial_update_command_string(self, serial):
        """ Should only be called after a boot

        Args:
          serial (str): string containing the serial of the rPi
        """
        self.clear_queue()
        self.enqueue_parameter_update("ID", serial)
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
        self.serial_connection = serial.Serial(port='/dev/serial0', baudrate=9600, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, xonxoff=False, rtscts=False, dsrdtr=False)

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
        """ Get uniqeu serial number
        Get a unique serial number from the cpu, shortened
        for easy readability.

        Args:
          None
        """
        serial = "0000000000"
        try:
            with open('/proc/cpuinfo', 'r') as fp:
                for line in fp:
                    if line[0:6] == 'Serial':
                        serial = line[10:26]
                        serial = serial.lstrip("0")
            if serial == "0000000000":
                raise TypeError('Could not extract serial from /proc/cpuinfo')
        except FileExistsError as err:
            serial = "0000000000"
            print(err.message)

        except TypeError as err:
            serial = "0000000000"
            print(err.message)
        return serial

    def get_iface_list(self):
        """ Get the list of interfaces
        
        Args:
          None
          
        Returns:
        """
        return "Not implemented" 
        
    def get_iface_hardware_address(self, iface):
        """ Get the hardware (MAC) address of the given interface
        
        Args:
          iface (str): interface to return the mac off.
          
        Returns:
          str: colon-delimited hardware address
        """
        try:
            mac = open('/sys/class/net/'+iface+'/address').readline()
        except:
            hex = uuid.getnode()
            mac = ':'.join(['{:02x}'.format((hex >> ele) & 0xff)
                              for ele in range(0, 8*6, 8)][::-1])

        return mac[0:17]
            
    def get_mac_address(self):
        """ Get the ethernet MAC address
        This returns in the MAC address in the canonical human-reable form

        Args:
          None
        """
        hex = uuid.getnode()
        formatted = ':'.join(['{:02x}'.format((hex >> ele) & 0xff)
                              for ele in range(0, 8*6, 8)][::-1])
        return formatted
        
    def get_ip_address(self):
        """ Get the primary IP address
        
        Args:
          None
        
        Returns:
          String: standard representation of the device's IP address
        """
        ip = commands.getoutput('hostname -I')
        ip = ip.rstrip()
        return ip


class Sensors():
    """ Class to handle sensor communication

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

        Args:
            None
        '''
        if (self.verbosity == 1):
            print("starting monitor")
        while True:
#            if self.arduino_link.serial_connection.in_waiting:
                try:
                    line = self.arduino_link.serial_connection.readline().rstrip()
                    if self.checksum_passed(line):
                        if (self.verbosity == 1):
                            print("checksum passed")
                        with self.lock:
                            self.save_message_dict(line)
                    else:
                        if (self.verbosity == 1):
                            print('Ign: ' + line.decode().rstrip())
                except BaseException as e:
                    time.sleep(1)
                    print('Error: ', e)
                    # return
#           else:
 #               print('Waiting... ' + format(self.arduino_link.serial_connection.in_waiting))        
  #              time.sleep(4)

    def pop_param(self, msg, char):
        ''' pop

        Args:
            char (char): a special character occuring only once
            msg (str): a string containing the the special character

        Returns:
            (sub_msg[0], sub_msg[1]) : two sub strings on either side of char.
            When there is more than one occurence of char (or when it is not present),
            returns the tuple (False, msg)
        '''

        msg = msg.decode().split(char)
        if len(msg) != 2:
            if (self.verbosity == 1):
                print("Corrupt message: while splitting with special character '{}' ".format(char))
            return False, msg
        return msg[0], msg[1]

    def checksum_passed(self, msg):
        ''' Check if the checksum passed
        recompute message checksum and compares with appended hash

        Args:
            msg (str): a string containing the message, having the following
                          format: Len~CRC32$Param|Value&Param|Value

        Returns:
            `True` when `msg` passes the checksum, `False` otherwise
        '''
        # pop the Len
        msg_len, msg = self.pop_param(msg, '~')
        if msg_len == False:
            return False
        # pop the Crc
        msg_crc, msg = self.pop_param(msg, '$')
        if msg_crc == False:
            return False

        # compare CRC32
        calcCRC = binascii.crc32(msg.rstrip()) & 0xffffffff
        if format(calcCRC, 'x') == msg_crc.lstrip("0"):
            if (self.verbosity == 1):
                print("CRC32 matches; message: " + msg)
            return True
        else:
            if (self.verbosity == 1):
                print(
                    "CRC32 Fail: calculated " +
                    format(calcCRC, 'x') +
                    " but received " +
                    msg_crc)
            return False

    def save_message_dict(self, msg):
        '''
        Takes the serial output from the incubator and creates a dictionary
        using the two letter ident as a key and the value as a value.

        Args:
            msg (string): The raw serial message (including the trailing checksum)

        Only use a message that passed the checksum!
        '''

        # pop the Len and CRC out
        tmp, msg = self.pop_param(msg, '$')

        if tmp == False:
            return False
        self.sensorframe = {}
        for params in msg.split('&'):
            kvp = params.split("|")
            if len(kvp) != 2:
                print("ERROR: bad key-value pair")
            else:
                self.sensorframe[kvp[0].encode("utf-8")] = kvp[1].encode("utf-8")
        self.sensorframe['Time'] = time.strftime(
            "%Y-%m-%d %H:%M:%S", time.gmtime())
        return
    
    def send_message_to_arduino(self, msg):
        '''
        Will transmit a string to the Arduino via the serial link.
        
        Args:
            msg (string): The message string to be sent to the Arduino, including checksums et all.
        '''
        
        try:
            self.arduino_link.serial_connection.write(msg.encode())
            self.arduino_link.serial_connection.flush()
        except BaseException as e:
            time.sleep(1)
            print('Error: ', e)

             

if __name__ == '__main__':
    print("PySerial version: " + serial.__version__)

    test_connections = False
    test_connections = True
    if test_connections:
        iface = Interface()
        print("Hardware serial number: {}".format(iface.get_serial_number()))
        print("Network interfaces: {}".format(iface.get_iface_list()))
        print("Hardware address (ethernet): {}".format(iface.get_iface_hardware_address("eth0")))
        print("Hardware address (wifi): {}".format(iface.get_iface_hardware_address("wlan0")))
        print("Primary IP address: {}".format(iface.get_ip_address()))
        print("Can connect to the internet: {}".format(iface.connects_to_internet()))
        print("Can contact the mothership: {}".format(
            iface.connects_to_internet(host='35.183.143.177', port=80)))
        print("Has serial connection with Arduino: {}".format(iface.serial_connection.is_open))
        del iface

    test_commandset = True
    if test_commandset:
        arduino_handler = ArduinoDirectiveHandler()
        mon = Sensors()

        msg0 = arduino_handler.get_arduino_serial_update_command_string(mon.arduino_link.get_serial_number())
        print("Sending message: " + msg0)
        mon.send_message_to_arduino(msg0)
        time.sleep(5)

        msg1 = arduino_handler.get_arduino_ip_update_command_string(mon.arduino_link.get_ip_address())
        print("Sending message: " + msg1)
        mon.send_message_to_arduino(msg1)
        time.sleep(5)
        
        msg2 = arduino_handler.get_arduino_mac_update_command_string("MWR", mon.arduino_link.get_iface_hardware_address("eth0"))
        print("Sending message: " + msg2)
        mon.send_message_to_arduino(msg2)
        time.sleep(5)
        
        msg3 = arduino_handler.get_arduino_mac_update_command_string("MWF", mon.arduino_link.get_iface_hardware_address("wlan0"))
        print("Sending message: " + msg3)
        mon.send_message_to_arduino(msg3)

        del arduino_handler

    monitor_serial = False
    monitor_serial = True
    if monitor_serial:
        mon = Sensors()
        mon.verbosity = 0
        print("Has serial connection with Arduino: {}".format(
            mon.arduino_link.serial_connection.is_open))
        print("Displaying serial data")
        while True:
            time.sleep(5)
            # print("trying...")
            print(mon.sensorframe)
            # print(mon.arduino_link.serial_connection.readline())
        del mon
