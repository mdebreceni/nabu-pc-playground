#!/usr/bin/env python3
# 
# NABU Adaptor Emulator - Copyright Mike Debreceni - 2022
# 
# Usage:   python3 ./nabu-adaptor-emu.py [--baudrate BAUDRATE] TTYNAME
#
# Example:
#          python3 ./nabu-adaptor-emu.py --baudrate=111863 /dev/ttyUSB0
#
# 
# This only supports a single NABU program, saved as '000001.PAK'.
# 

import argparse
import serial
import time
from nabu_pak import NabuSegment, NabuPack

# request type
# Others (from http://dunfield.classiccmp.org/nabu/nabutech.pdf,
# Segment Routines  Page 9 - 3
# 
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


def send_ack():
    sendBytes(bytes([0x10, 0x06]))

# TODO:  We can probably get rid of handle_0xf0_request, handle_0x0f_request and handle_0x03_request
# TODO:  as these bytes may have been from RS-422 buffer overruns / other errors

def handle_0xf0_request(data):  
    sendBytes(bytes([0xe4]))

def handle_0x0f_request(data):
    sendBytes(bytes([0xe4]))

def handle_0x03_request(data):
    sendBytes(bytes([0xe4]))

def handle_reset_segment_handler(data):
    sendBytes(bytes([0x10, 0x06, 0xe4]))

def handle_reset(data):
    send_ack()

def handle_get_status(data):
    global channelCode
    send_ack()

    response = recvBytes()
    if channelCode is None:
        print("* Channel Code is not set yet.")
        # Ask NPC to set channel code
        sendBytes(bytes([0x9f, 0x10, 0xe1]))
    else:
        print("* Channel code is set to " + channelCode)
        # Report that channel code is already set
        sendBytes(bytes([0x1f, 0x10, 0xe1]))

def handle_set_status(data):
    sendBytes(bytes([0x10, 0x06, 0xe4]))

def handle_download_segment(data):
    # ; segment load request
    # [11]        NPC       $84
    #              NA        $10 06
    send_ack()
    packetNumber=recvBytesExactLen(1)[0]
    segmentNumber=bytes(reversed(recvBytesExactLen(3)))
    segmentId=str(segmentNumber.hex())
    print("* Requested Segment ID: " + segmentId)
    print("* Requested PacketNumber: " + str(packetNumber))

    sendBytes(bytes([0xe4, 0x91]))

    response = recvBytes(2)
    print("* Response from NPC: " + response.hex(" "))

    # Get Segment from internal segment store
    if segmentId in segments:
        segment = segments[segmentId]
    else:
        return None
   
    # Get requested pack from that segment
    pack_data = segment.get_pack(packetNumber)

    # Dump information about pack.  'pack' is otherwise unused
    pack = NabuPack()
    pack.ingest_bytes(pack_data)
    print("* Pack to send: " + pack_data.hex(' ')) 
    print("* pack_segment_id: "+ pack.pack_segment_id.hex())
    print("* pack_packnum: " + pack.pack_packnum.hex())
    print("* segment_owner: " + pack.segment_owner.hex())
    print("* segment_tier: " + pack.segment_tier.hex())
    print("* segment_mystery_bytes: " + pack.segment_mystery_bytes.hex())
    print("* pack type: " + pack.pack_type.hex())
    print("* pack_number: " + pack.pack_number.hex())
    print("* pack_offset: " + pack.pack_offset.hex())
    print("* pack_crc: " + pack.pack_crc.hex())
    print("* pack length: {}".format(len(pack_data)))

    # escape pack data (0x10 bytes should be escaped maybe?)
    escaped_pack_data = escapeUploadBytes(pack_data)
    sendBytes(escaped_pack_data)
    sendBytes(bytes([0x10, 0xe1]))

def handle_set_channel_code(data):
    global channelCode
    send_ack()
    data = recvBytesExactLen(2)
    while len(data) < 2:
        remaining = 2 - len(data)
        print("Waiting for channel code")
        print(data.hex(' '))
        data = data + recvBytes(remaining)

    print("* Received Channel code bytes: " + data.hex())
    channelCode = bytes(reversed(data)).hex()
    print("* Channel code: " + channelCode)
    sendBytes(bytes([0xe4]))

def handle_0x8f_req(data):
    print("* 0x8f request")
    data = recvBytes()
    sendBytes(bytes([0xe4]))

def handle_unimplemented_req(data):
    print("* ??? Unimplemented request")
    print("* " + data.hex(' '))

def escapeUploadBytes(data):
    escapedBytes = bytearray()

    for idx in range(len(data)):
        byte=data[idx]
        if(byte == 0x10):
            escapedBytes.append(byte)
            escapedBytes.append(byte)
        else:
            escapedBytes.append(byte)

    return escapedBytes

def sendBytes(data):
    chunk_size=6
    index=0
    delay_secs=0
    end=len(data)

    while index + chunk_size < end:
        ser.write(data[index:index+chunk_size])
#        print("NA-->NPC:  " + data[index:index+chunk_size].hex(' '))
        index += chunk_size
        time.sleep(delay_secs)

    if index != end:
#        print("NA-->NPC:  " + data[index:end].hex(' '))
        ser.write(data[index:end])

def recvBytesExactLen(length=None):
    if(length is None):
        return None
    data = recvBytes(length)
    while len(data) < length:
        remaining = length - len(data)
#        print("Waiting for {} more bytes".format(length - len(data)))
        print(data.hex(' '))
        time.sleep(0.01)
        data = data + recvBytes(remaining)
    return data

def recvBytes(length = None):
    if(length is None):
        data = ser.read(MAX_READ)
    else:
        data = ser.read(length)
    if(len(data) > 0):
        print("NPC-->NA:   " + data.hex(' '))
    return data


######  Begin main code here


MAX_READ=65535
DEFAULT_BAUDRATE=111863
# channelCode = None
channelCode = '0000'

parser = argparse.ArgumentParser()
# Positional argument for ttyname - required
parser.add_argument("ttyname",
        help="Set serial device (e.g. /dev/ttyUSB0)")
# Optional argument for baudrate
parser.add_argument("-b", "--baudrate",
        type=int,
        help="Set serial baud rate (default: {} BPS)".format(DEFAULT_BAUDRATE),
        default=DEFAULT_BAUDRATE)
args = parser.parse_args()

if args.ttyname is None:
    parser.print_help()
    exit(1)

# Some hard-coded things here (timeout, stopbits)
ser = serial.Serial(port=args.ttyname, baudrate=args.baudrate, timeout=0.5, stopbits=serial.STOPBITS_TWO)

segments = {}

print("* Loading NABU Segments into memory")
segment1 = NabuSegment()

# Crude implementation - we could probably scan a directory for files here.
# TODO: The PAK file is pre-split into individual packets, each with headers and checksums. 
# TODO: We should change this to handle .nabu files instead, which have not yet been split into packets with headers and checksums
# 
segment1.ingest_from_file("000001.PAK")
segments["000001"] = segment1

while True:
    data = recvBytes()
    if len(data) > 0:
        req_type = data[0]

        if req_type == 0x03:
            print("* 0x03 request")
            handle_0x03_request(data)
        elif req_type == 0x0f:
            print("* 0x0f request")
            handle_0x0f_request(data)
        elif req_type == 0xf0:
            print("* 0xf0 request")
            handle_0xf0_request(data)
        elif req_type == 0x80:
            print("* Reset segment handler")
            handle_reset_segment_handler(data)
        elif req_type == 0x81:
            print("* Reset")
            handle_reset(data)
        elif req_type == 0x82:
            print("* Get Status")
            handle_get_status(data)
        elif req_type == 0x83:
            print("* Set Status")
            handle_set_status(data)
        elif req_type == 0x84:
            print("* Download Segment Request")
            handle_download_segment(data)
        elif req_type == 0x85:
            print("* Set Channel Code")
            handle_set_channel_code(data)
            print("* Channel code is now " + channelCode)
        elif req_type == 0x8f:
            print("* Handle 0x8f")
            handle_0x8f_req(data)
        else:
            print("* Req type {} is Unimplemented :(".format(data[0]))
            handle_unimplemented_req(data)

