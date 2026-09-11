import serial

ser = serial.Serial("COM19", 9600, timeout=1)
print("Port offen:", ser.is_open)
ser.close()
