import serial
import time

PORT = "COM14"

# CH340/Windows zunächst mit Standardbaudrate initialisieren
wake = serial.Serial(PORT, 9600, timeout=1)
time.sleep(0.5)
wake.close()

time.sleep(0.2)

# Anschließend als DMX-Port öffnen
ser = serial.Serial(
    PORT,
    250000,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_TWO,
    timeout=0
)
