import pyvjoy
import serial


class VJoyController:
    def __init__(self, device_id=1):
        self.joystick = pyvjoy.VJoyDevice(device_id)
        self.mapping = {
            'B': ('button', 0),
            'Y': ('button', 1),
            'X': ('button', 2),
            'A': ('button', 3),
            'TR': ('button', 4),  # right trigger
            'TL': ('button', 5),  # left trigger
            'CJX': ('axis', pyvjoy.HID_USAGE_RX),  # camera stick x
            'CJY': ('axis', pyvjoy.HID_USAGE_RY),  # camera stick y
            'MJX': ('axis', pyvjoy.HID_USAGE_X),  # movement stick x
            'MJY': ('axis', pyvjoy.HID_USAGE_Y)  # movement stick y
        }

    def process_input(self, axis, value):
        action_type, action_index = self.mapping[axis]
        if action_type == 'button':
            self.joystick.set_button(action_index + 1, value != 0)
        elif action_type == 'axis':
            scaled_value = int(value * 32767 / 1000 + 32768)
            self.joystick.set_axis(action_index, scaled_value)


class BluetoothReceiver:
    def __init__(self, com_port='COM3', baud_rate=9600):
        self.serial = serial.Serial(com_port, baud_rate)

    def read_data(self):
        while True:
            data = self.serial.read(1)
            if data == b'\xFF':
                break
        return self.serial.read(3)


def parse_data(data):
    axis = data[0]
    value = int.from_bytes(data[1:3], byteorder='little', signed=True)
    print("received data: ", axis, value)
    return axis, value


def main():
    receiver = BluetoothReceiver()
    controller = VJoyController()

    try:
        while True:
            print("waiting for controller input...")
            data = receiver.read_data()
            axis, value = parse_data(data)
            controller.process_input(axis, value)

    except KeyboardInterrupt:
        print("program terminated by user. exiting...")
    except Exception as e:
        print("error occurred: ", e)
    finally:
        receiver.serial.close()


if __name__ == "__main__":
    main()
