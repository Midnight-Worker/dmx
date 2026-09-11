import serial
import time

PORT = "COM19"

print(f"Initialisiere {PORT} zunächst mit 9600 Baud ...")

ser = serial.Serial(
    port=PORT,
    baudrate=9600,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    timeout=1
)

time.sleep(0.5)

# Ohne Schließen auf DMX umstellen
ser.baudrate = 250000
ser.bytesize = serial.EIGHTBITS
ser.parity = serial.PARITY_NONE
ser.stopbits = serial.STOPBITS_ONE
ser.timeout = 0

# Startcode + 6 DMX-Kanäle
frame = bytearray([
    0,      # DMX-Startcode
    255,    # Kanal 1: Master
    255,    # Kanal 2: Rot
    0,      # Kanal 3: Grün
    0,      # Kanal 4: Blau
    0,      # Kanal 5
    0       # Kanal 6
])

print(f"DMX läuft auf {PORT}: Master=255, Rot=255")
print("CTRL+C zum Beenden")

try:
    while True:
        # DMX BREAK: mindestens 88 µs
        ser.break_condition = True
        time.sleep(0.001)

        # MARK AFTER BREAK: mindestens 8 µs
        ser.break_condition = False
        time.sleep(0.001)

        # Startcode und Kanäle senden
        ser.write(frame)
        ser.flush()

        time.sleep(0.03)

except KeyboardInterrupt:
    print("\nDMX beendet.")

finally:
    ser.break_condition = False
    ser.close()
