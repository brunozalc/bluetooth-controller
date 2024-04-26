import pyvjoy
import serial
import threading


## ----------------- vjoy controller ----------------- ##
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
            9: ('axis', pyvjoy.HID_USAGE_Y),   # MJY (left stick y-axis)
            10: ('button', 6),  # SWR (right joystick switch)
            11: ('button', 7),  # SWL (left joystick switch)
        }

        self.release_delays = {}
        self.button_release_delay = 0.1  # seconds

    def process_input(self, idx, value):
        if idx not in self.mapping:
            raise ValueError("invalid control index")

        action_type, action_index = self.mapping[idx]

        if action_type == 'button':
            if value not in [0, 1]:
                raise ValueError("button value must be 0 or 1")
            
            self.gamepad.set_button(action_index, value != 0)
            if value == 1:
                if action_index in self.release_delays:
                    self.release_delays[action_index].cancel() 
                timer = threading.Timer(self.button_release_delay, self.reset_button, [action_index])
                timer.start()
                self.release_delays[action_index] = timer

        elif action_type == 'axis':
            scaled_value = int((32767 / 510) * (value + 255))
            self.gamepad.set_axis(action_index, scaled_value)
    
    def reset_button(self, button_id):
        self.gamepad.set_button(button_id, False)


## ----------------- bluetooth receiver ----------------- ##
class BluetoothReceiver:
    def __init__(self, com_port='COM5', baud_rate=9600):
        self.serial = serial.Serial(com_port, baud_rate)

    def read_data(self):
        buffer = b''
        while len(buffer) < 4:
            buffer += self.serial.read(4 - len(buffer))
            if buffer[-1] == 255:
                if len(buffer) == 4:
                    return buffer[:3]
                else:
                    buffer = b''  # reset buffer if packet end misaligned
            elif len(buffer) > 4:
                raise ValueError("packet length exceeded")
        raise ValueError("invalid packet end")


## ----------------- main program ----------------- ##
def parse_data(data):
    idx = data[0]
    value = int.from_bytes(data[1:3], byteorder='big', signed=True)
    print("received data: IDX = {}, VALUE = {}".format(idx, value))
    return idx, value


def main():
    receiver = BluetoothReceiver()
    controller = VJoyController()

    try:
        while True:
            try:
                data = receiver.read_data()
                idx, value = parse_data(data)
                controller.process_input(idx, value)
            except Exception as e:
                print(f"\nnon-fatal error occurred: {e}\n")

    except Exception as e:
        print(f"\nerror occurred: {e}\n")            
    except KeyboardInterrupt:
        print("\nprogram terminated by user. exiting...\n")
    finally:
        receiver.serial.close()
        

if __name__ == "__main__":
    main()
