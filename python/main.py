import pyvjoy
import time


def process_input(idx, value):
    # initialize the vJoy device
    j = pyvjoy.VJoyDevice(1)

    if idx <= 3:  # button index is from 0 to 3
        j.set_button(idx + 1, value != 0)
    elif idx == 4:  # x axis of the joystick
        j.set_axis(pyvjoy.HID_USAGE_X, int(value * 32767 / 1000 + 32768))
    elif idx == 5:  # y axis of the joystick
        j.set_axis(pyvjoy.HID_USAGE_Y, int(value * 32767 / 1000 + 32768))


def main():
    simulated_data = [
        (0, 1),   # button 1 pressed
        (0, 0),   # button 1 released (
        (1, 1),   # button 2 pressed
        (1, 0),   # button 2 released
        (2, 1),   # button 3 pressed
        (2, 0),   # button 3 released
        (3, 1),   # button 4 pressed
        (4, 500),  # joystick x axis at 500
        (4, -500),  # joystick x axis at -500
        (5, 500),  # joystick y axis at 500
        (5, -500)  # joystick y axis at -500
    ]

    while True:
        for idx, value in simulated_data:
            process_input(idx, value)
            time.sleep(0.1)
            print("simulated data sent to vJoy device")


if __name__ == "__main__":
    main()
