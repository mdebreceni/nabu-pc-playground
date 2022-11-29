#!/usr/bin/env python3
import serial
import time



# request type
# Others (from http://dunfield.classiccmp.org/nabu/nabutech.pdf,
# Segment Routines  Page 9 - 3
# $80   Reset Segment Handler (Resets Adaptor to known state)
#       (also see Page 2 - 14)
# $81   Reset (?)
# $82   Get adaptor status
# $83   Set adaptor status
# $84   Download segment
# $85   Set channel code

# Others (from http://dunfield.classiccmp.org/nabu/nabutech.pdf,
# $87   SEG$CST ZBase Address - return control status block
# $88   Directory Search
# $8f   Unknown
# $96   Load XIOS Module
# $97   Unload XIOS Module
# $99   Resolve Global Reference

MAX_READ=65535

def send_ack():
    sendBytes(bytes([0x10, 0x06]))

def handle_reset_segment_handler(data):
    sendBytes(bytes([0x10, 0x06, 0xe4]))

def handle_reset(data):
    send_ack()

def handle_get_status(data):
    send_ack()
    # wait for 0x01
    # expectBytes([0x01])
    response = recvBytes()
    # then send respnse
    sendBytes(bytes([0x9f, 0x10, 0xe1]))


def handle_set_status(data):
    sendBytes(bytes([0x10, 0x06, 0xe4]))

def handle_download_segment(data):
    send_ack()
# ; segment load request
# [11]        NPC       $84
#              NA        $10 06
    packetNumber=recvBytes(1)
    print("PacketNumber: " + packetNumber.hex())
    segmentNumber=bytes(reversed(recvBytes(3)))
    print("SegmentNumber: " + segmentNumber.hex())

    sendBytes(bytes([0xe4, 0x91]))

# [12]        NPC       $00 $01 $00 $00       pocket number byte (0)
# followed by segment number (lsb first)
#              NA        $e4 $91
# [13]        NPC       $10 $06
#              NA        sends a packet bytes + $10 $e1 at the end
# [14]        NPC       $84
#              NA        sendes the next segment + $10 $06
# [15]        NPC       $01 $01 $00 $00   (the first byte beinng packet nr)
#              NA        $e4 $91
# [16]        NPC       $10 $06
#              NA        packet bytes + $10 $e1 at the end

# .............  and so on ..................
#
# ; last packet
# [175]      NPC       $10 $06
#             NA        packet bytes + $10 $e1 at the end
# [176]      NPC       $81
#             NA        $10 $06
# [177]      NPC       $0f $05
#             NA e4




def handle_set_channel_code(data):
    # when would channel code be set?  How does NPC know to do this?  How does NA respond    # if channel code is set vs. not set?
    send_ack()
    # acceptBytes([2 bytes for channel code])
    time.sleep(0.5)

    channelCode = recvBytes(2)
    while len(channelCode) == 0:
        print("Waiting for channel code")
        channelCode = recvBytes()

    print("Channel code: " + channelCode.hex())
    channelCode = bytes(reversed(channelCode))
    print("Channel code: " + channelCode.hex())
    sendBytes(bytes([0xe4]))
    # ...
def handle_0x8f_req(data):
    print("0x8f request")
    sendBytes(bytes([0xe4]))

def handle_unimplemented_req(data):
    print("???")
    print(data.hex(' '))

def sendBytes(data):
    print("TX " + data.hex(' '))
    ser.write(data)

def recvBytes(length = None):
    if(length is None):
      data = ser.read()
    else:
      data = ser.read(length)
    if(len(data) > 0):
      print("RX " + data.hex(' '))
    return data

ser = serial.Serial(port='/dev/ttyUSB0', baudrate=115200, timeout=0.1)



while True:
    data = recvBytes()
    if len(data) > 0:
        req_type = data[0]

        if req_type == 0x80:
            print("Reset segment handler")
            handle_reset_segment_handler(data)
        elif req_type == 0x81:
            print("Reset")
            handle_reset(data)
        elif req_type == 0x82:
            print("Get Status")
            handle_get_status(data)
        elif req_type == 0x83:
            print("Set Status")
            handle_set_status(data)
        elif req_type == 0x84:
            print("Download Segment Request")
            handle_download_segment(data)
        elif req_type == 0x85:
            print("Set Channel Code")
            handle_set_channel_code(data)
        elif req_type == 0x8f:
            print("Handle 0x8f")
            handle_0x8f_req(data)
        else:
            print("Unimplemented :(")
            handle_unimplemented_req(data)
    time.sleep(0.1)




# ; message nr  device    hex code
# [1]         NPC       $83
#              NA        $10 $06 $e4

# [2]         NPC       $82
#              NA        $10 $06
# [3]         NPC       $01
#              NA        $9f $10 $e1

# [4]         NPC       $83
#              NA        $10 $06 $e4

# [5]         NPC       $82
#              NA        $10 $06
# [6]         NPC       $01
#              NA        $9f $10 $e1


# ; typing channel code
# [7]         NPC       $85
#              NA        $10 $06
# [8]         NPC       $00 $00    channel code
#              NA        $e4
# [9]         NPC       $81
#              NA        $10 $06
# [10]        NPC       $8f $05
#              NA        $e4
# ; segment load request
# [11]        NPC       $84
#              NA        $10 06
# [12]        NPC       $00 $01 $00 $00       pocket number byte (0)
# followed by segment number (lsb first)
#              NA        $e4 $91
# [13]        NPC       $10 $06
#              NA        sends a packet bytes + $10 $e1 at the end
# [14]        NPC       $84
#              NA        sendes the next segment + $10 $06
# [15]        NPC       $01 $01 $00 $00   (the first byte beinng packet nr)
#              NA        $e4 $91
# [16]        NPC       $10 $06
#              NA        packet bytes + $10 $e1 at the end
#
# .............  and so on ..................
#
# ; last packet
# [175]      NPC       $10 $06
#             NA        packet bytes + $10 $e1 at the end
# [176]      NPC       $81
#             NA        $10 $06
# [177]      NPC       $0f $05
#             NA e4
