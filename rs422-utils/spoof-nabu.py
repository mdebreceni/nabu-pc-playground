import serial
import time

ser = serial.Serial(port='/dev/ttyUSB0',baudrate=115200,timeout=0.5)
print(ser.BAUDRATES)
while True:
    data = ser.read(10000)

    data = bytes([0x83])
    print('tx: ' + data.hex(' '))
    ser.write(data)
    ser.flush()
   
    data = bytes([])
    while len(data) == 0:
      data = ser.read(10000)
    print('rx: ' + data.hex(' '))


    data = bytes([0x83])
    print('tx: ' + data.hex(' '))
    ser.write(data)
    ser.flush()
   
    data = bytes([])
    while len(data) == 0:
      data = ser.read(10000)
    print('rx: ' + data.hex(' '))



    data = bytes([0x82]);
    print('tx: ' + data.hex(' '))
    ser.write(data)

    data = bytes([])
    while len(data) == 0:
      data = ser.read(10000)
    print('rx: ' + data.hex(' '))
