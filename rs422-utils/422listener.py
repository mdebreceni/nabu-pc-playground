import serial
import time

ser = serial.Serial(port='/dev/ttyUSB0',baudrate=115200)
print(ser.BAUDRATES)
while True:
    data = ser.read()
    print("Rx: " + data.hex())
