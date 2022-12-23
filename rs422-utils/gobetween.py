import serial
import time

ser0 = serial.Serial(port='/dev/ttyUSB0',baudrate=115200, timeout=0.01)
ser1 = serial.Serial(port='/dev/ttyUSB1',baudrate=115200, timeout=0.01)
lastread = None


while True:
    data = ser0.read()

    if data != None and len(data) > 0:
        if lastread != "ser0":
            print("--------")
            lastread = "ser0"
        print("ser0->ser1: ", data.hex())
        ser1.write(data)
    
    ser1.flush()
    data = ser1.read()
    if data != None and len(data) > 0:
        if lastread != "ser1":
            print("--------")
            lastread = "ser1"
        print("ser1->ser0: ", data.hex())
        ser0.write(data)
    ser0.flush()


