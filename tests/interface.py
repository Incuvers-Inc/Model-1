import common
import monitor
import unittest


class TestInterface(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        self.iface = monitor.Interface()

    def tearDown(self):
        del self.iface

    def test_serial_number(self):
        # print("Hardware serial number: {}".format(iface.get_serial_number()))
        # this is only valid for the demo unit
        assertEqual(iface.get_serial_number(), "00000000f8aa31fe")

    def test_get_mac_address(self):
        # print("Hardware address (ethernet): {}".format(iface.get_mac_address()))
        assertEqual(iface.get_mac_address(), "b8:27:eb:ff:64:ab")

    def test_internet_connection(self):
        # print("Can contact the mothership: {}".format(iface.connects_to_internet()))
        assertTrue(iface.connects_to_internet())
        # print("Can contact the mothership: {}".format(iface.connects_to_internet(host = '35.183.143.177', port = 80)))
        assertTrue(iface.connects_to_internet(host='35.183.143.177', port=80))

    def serial_connection(self):
        # print("Has serial connection with Arduino: {}".format(iface.serial_connection.is_open()))
        assertTrue(iface.serial_connection.is_open())


if __name__ == '__main__':
    unittest.main()
