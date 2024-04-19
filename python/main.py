import pyvjoy
import serial


class VJoyController:
    def __init__(self, device_id=1):
        self.gamepad = pyvjoy.VJoyDevice(device_id)
        self.mapping = {
            0: ('button', 0),  # B
            1: ('button', 1),  # Y
            2: ('button', 2),  # X
            3: ('button', 3),  # A
            4: ('button', 4),  # TR (right trigger)
            5: ('button', 5),  # TL (left trigger)
            6: ('axis', pyvjoy.HID_USAGE_RX),  # CJX (right stick x-axis)
            7: ('axis', pyvjoy.HID_USAGE_RY),  # CJY  (right stick y-axis)
            8: ('axis', pyvjoy.HID_USAGE_X),  # MJX (left stick x-axis)
            9: ('axis', pyvjoy.HID_USAGE_Y)   # MJY (left stick y-axis)
        }

    def process_input(self, idx, value):
        if idx not in self.mapping:
            raise ValueError("invalid control index")

        action_type, action_index = self.mapping[idx]

        if action_type == 'button':
            if value not in [0, 1]:
                raise ValueError("button value must be 0 or 1")
            self.gamepad.set_button(action_index + 1, value != 0)
        elif action_type == 'axis':
            scaled_value = int(value * 32767 / 1000 + 32768)
            self.gamepad.set_axis(action_index, scaled_value)


class BluetoothReceiver:
    def __init__(self, com_port='/dev/rfcomm0', baud_rate=9600):
        self.serial = serial.Serial(com_port, baud_rate)

    def read_data(self):
        data = self.serial.read(4)
        if data[-1] != 255:
            raise ValueError("invalid packet end")
        return data[:3]


def parse_data(data):
    idx = data[0]
    value = int.from_bytes(data[1:3], byteorder='big', signed=True)
    print("received data: IDX={}, VALUE={}".format(idx, value))
    return idx, value


def main():
    receiver = BluetoothReceiver()
    controller = VJoyController()

    try:
        while True:
            print("waiting for controller input...")
            data = receiver.read_data()
            idx, value = parse_data(data)
            controller.process_input(idx, value)

    except KeyboardInterrupt:
        print("program terminated by user. exiting...")
    except Exception as e:
        print("error occurred: ", e)
    finally:
        receiver.serial.close()


if __name__ == "__main__":
    main()
