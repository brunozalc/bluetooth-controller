import unittest
import pyvjoy
from unittest.mock import MagicMock, patch
from main import VJoyController, BluetoothReceiver, parse_data

# this checks if the controller is being created and is able to process inputs via vjoy driver


class TestVJoyController(unittest.TestCase):
    @patch('pyvjoy.VJoyDevice')
    def test_process_input_buttons(self, mock_vjoy):
        # setup
        controller = VJoyController()
        controller.gamepad = mock_vjoy

        # test button press
        controller.process_input(0, 1)  # index 0 for 'B'
        mock_vjoy.set_button.assert_called_once_with(1, True)

        # test button release
        controller.process_input(0, 0)  # index 0 for 'B'
        mock_vjoy.set_button.assert_called_with(1, False)

    @patch('pyvjoy.VJoyDevice')
    def test_process_input_axes(self, mock_vjoy):
        # setup
        controller = VJoyController()
        controller.gamepad = mock_vjoy

        # test axis movement
        controller.process_input(6, 500)  # index 6 for 'CJX'
        scaled_value = int(500 * 32767 / 1000 + 32768)
        mock_vjoy.set_axis.assert_called_once_with(
            pyvjoy.HID_USAGE_RX, scaled_value)


# this checks if the bluetooth receiver is able to read data from the serial port
# if this passes and somehow the connection is not working, it's probably a hardware/serial port issue
class TestBluetoothReceiver(unittest.TestCase):
    @patch('serial.Serial')
    def test_read_data(self, mock_serial):
        # setup
        mock_serial.return_value.read.return_value = b'\x01\x02\x03\xFF'
        receiver = BluetoothReceiver()

        # action
        data = receiver.read_data()

        # test
        # EOP is checked but not returned
        self.assertEqual(data, b'\x01\x02\x03')

# this checks if the data is being parsed correctly after being received from bluetooth


class TestParseData(unittest.TestCase):
    def test_parse_data(self):
        # mock data bytes
        data = b'\x01\x01\x00'  # example data: idx 1, value = 1 (button press)
        idx, value = parse_data(data)

        # assertions
        self.assertEqual(idx, 1)
        self.assertEqual(value, 1)


if __name__ == '__main__':
    unittest.main()
