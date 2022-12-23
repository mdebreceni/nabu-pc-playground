import serial
import time

ser = serial.Serial(port='/dev/ttyUSB1',baudrate=115200)
print(ser.BAUDRATES)
while True:
    data=bytes([0x01,0x02,0x03])
    ser.write(data)
    print("Tx: " + data.hex())
    time.sleep(0.2)
