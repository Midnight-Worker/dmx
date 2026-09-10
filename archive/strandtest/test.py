import serial
import time

PORT = "COM15"

ser = serial.Serial(
    port=PORT,
    baudrate=250000,
    bytesize=8,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_TWO,
    timeout=0
)

# Startcode + 6 DMX-Kanäle
frame = bytearray(7)

frame[0] = 0     # DMX Startcode

frame[1] = 0   # Kanal 1: Rot
frame[2] = 0    # Kanal 2: Grün
frame[3] = 0     # Kanal 3: Blau
frame[4] = 0     # Kanal 4
frame[5] = 0     # Kanal 5
frame[6] = 0     # Kanal 6

print("DMX läuft. CTRL+C zum Beenden.")

try:
    while True:

        # BREAK
        ser.break_condition = True
        time.sleep(0.001)

        # MARK AFTER BREAK
        ser.break_condition = False
        time.sleep(0.001)

        # Startcode + Kanäle
        ser.write(frame)
        ser.flush()

        # ca. 30 Frames/s
        time.sleep(0.03)

except KeyboardInterrupt:
    pass

finally:
    ser.close()
