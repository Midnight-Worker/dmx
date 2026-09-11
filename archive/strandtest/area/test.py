import serial

PORT = "COM19"  # anpassen

tests = [
    (9600,   serial.STOPBITS_ONE, "9600 8N1"),
    (9600,   serial.STOPBITS_TWO, "9600 8N2"),
    (250000, serial.STOPBITS_ONE, "250000 8N1"),
    (250000, serial.STOPBITS_TWO, "250000 8N2"),
]

for baud, stopbits, name in tests:
    try:
        ser = serial.Serial(
            port=PORT,
            baudrate=baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=stopbits,
            timeout=1
        )
        print(f"OK:     {name}")
        ser.close()
    except Exception as e:
        print(f"FEHLER: {name}")
        print(f"        {e}")
