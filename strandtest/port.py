import serial

ser = serial.Serial(
    "COM16",
    250000,
    bytesize=8,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    timeout=1
)

print("250000 8N1 OK")
ser.close()
