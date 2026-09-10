import serial
import time

PORT = "COM15"

ser = serial.Serial(
    PORT,
    baudrate=250000,
    bytesize=8,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_TWO
)

frame = bytearray(7)
frame[0] = 0


def send_dmx():
    ser.break_condition = True
    time.sleep(0.001)

    ser.break_condition = False
    time.sleep(0.001)

    ser.write(frame)
    ser.flush()


def send_for(seconds=3):
    end = time.time() + seconds

    while time.time() < end:
        send_dmx()
        time.sleep(0.03)


try:
    while True:

        for channel in range(1, 7):

            # alles aus
            for i in range(1, 7):
                frame[i] = 0

            frame[channel] = 255

            print(f"Teste Kanal {channel} = 255")

            send_for(3)

            input("ENTER für nächsten Kanal...")

except KeyboardInterrupt:
    pass

finally:
    for i in range(1, 7):
        frame[i] = 0

    for _ in range(5):
        send_dmx()

    ser.close()

