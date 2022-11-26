import serial
import time

ser = serial.Serial(port='/dev/ttyUSB0',baudrate=115200)
print(ser.BAUDRATES)
while True:
    data = ser.read()
    print("<<" + data.hex())

    time.sleep(0.2)
    data = bytes([0x10, 0x06, 0xe4])
    ser.write(data)
    ser.flush()
