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
# Ugly hack to make this work with directory of pak files from a cycle added by Sark
# There is a better way to do this, I'm sure of it. But this works (mostly)
# This works with a directory of pak files, in paks/
# menu is 000001.pak, other files all upercase hex names with .pak extension

import argparse
import serial
import time
import datetime
from nabu_pak import NabuSegment, NabuPack
from crccheck.crc import Crc16Genibus
import asyncio

NABU_STATE_AWAITING_REQ = 0
NABU_STATE_PROCESSING_REQ = 1
MAX_READ=65535

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

class NabuAdaptor():
    def __init__(self, reader, writer):
        print("Called NabuAdaptor::__init__")
        self.reader=reader
        self.writer=writer

    async def run_NabuSession(self):
        while True:
            data = await self.recvBytes()
            if len(data) > 0:
                req_type = data[0]
                if req_type == 0x03:
                    print("* 0x03 request")
                    await self.handle_0x03_request(data)
                elif req_type == 0x05:
                    print("* 0x05 request")
                    await self.handle_0x05_request(data)
                elif req_type == 0x06:
                    print("* 0x06 request")
                    await self.handle_0x06_request(data)
                elif req_type == 0x0f:
                    print("* 0x0f request")
                    await self.handle_0x0f_request(data)
                elif req_type == 0xf0:
                    print("* 0xf0 request")
                    self.andle_0xf0_request(data)
                elif req_type == 0x80:
                    print("* Reset segment handler")
                    await self.handle_reset_segment_handler(data)
                elif req_type == 0x81:
                    print("* Reset")
                    await self.handle_reset(data)
                elif req_type == 0x82:
                    print("* Get Status")
                    await self.handle_get_status(data)
                elif req_type == 0x83:
                    print("* Set Status")
                    await self.handle_set_status(data)
                elif req_type == 0x84:
                    print("* Download Segment Request")
                    await self.handle_download_segment(data)
                elif req_type == 0x85:
                    print("* Set Channel Code")
                    await self.handle_set_channel_code(data)
                    print("* Channel code is now " + channelCode)
                elif req_type == 0x8f:
                    print("* Handle 0x8f")
                    await self.handle_0x8f_req(data)
                elif req_type == 0x10:
                    print("got request type 10, sending time")
                    await self.send_time()
                    await self.sendBytes(bytes([0x10, 0xe1]))
        
                else:
                    print("* Req type {} is Unimplemented :(".format(data[0]))
                    await self.handle_unimplemented_req(data)
    
    async def send_ack(self):
        await self.sendBytes(bytes([0x10, 0x06]))

    # Pre-formed time packet, sends Jan 1 1984 at 00:00:00
    # Todo - add proper time packet generation and CRC 

    def get_time_bytes(self):
        data = bytearray([0x7f, 0xff, 0xff, 0x00, 0x01, 0x7f, 0xff, 0xff, 0xff, 0x7f, 0x80, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02])
        now = time.localtime()
        data.append(now.tm_wday + 2)  # 2 seems to be the correct offset, if 1=Sun, 2=Mon, 3=Tue, etc...
        data.append(0x54) # This is 1984, forever
        data.append(now.tm_mon)
        data.append(now.tm_mday)
        data.append(now.tm_hour)
        data.append(now.tm_min)
        data.append(now.tm_sec)
        crc = Crc16Genibus.calc(data)   # Empirically gives us the right CRC
        data.append(crc >> 8 & 0xff)
        data.append(crc & 0xff)
        return bytes(data)

    async def send_time(self):
        time_bytes = self.get_time_bytes()
        escaped_time_bytes = self.escapeUploadBytes(time_bytes)
        await self.sendBytes(escaped_time_bytes)


    # TODO:  We can probably get rid of handle_0xf0_request, handle_0x0f_request and handle_0x03_request
    # TODO:  as these bytes may have been from RS-422 buffer overruns / other errors

    async def handle_0xf0_request(self, data):  
        await self.send_ack()
    
    async def handle_0x05_request(self, data):
        await self.send_ack()

    async def handle_0x06_request(self, data):
        await self.send_ack()

    async def handle_0x0f_request(self, data):
        await self.send_ack()

    async def handle_0x03_request(self, data):
        await self.send_ack()

    async def handle_reset_segment_handler(self, data):
        await self.sendBytes(bytes([0x10, 0x06, 0xe4]))

    async def handle_reset(self, data):
        await self.send_ack()

    async def handle_get_status(self, data):
        global channelCode
        await self.send_ack()
        response = await self.recvBytes()
        print("Response is of type {}".format(type(response)))
        print("Len(response) = {}".format(len(response)))
        print("Sent ack")
        if channelCode is None:
            print("* Channel Code is not set yet.")
            # Ask NPC to set channel code
            await self.sendBytes(bytes([0x9f, 0x10, 0xe1]))
        else:
            print("* Channel code is set to " + channelCode)
            # Report that channel code is already set
            await self.sendBytes(bytes([0x1f, 0x10, 0xe1]))

    async def handle_set_status(self, data):
        await self.sendBytes(bytes([0x10, 0x06, 0xe4]))

    async def handle_download_segment(self, data):
        # ; segment load request
        # [11]        NPC       $84
        #              NA        $10 06
        await self.send_ack()
        data=await self.recvBytesExactLen(1)
        packetNumber=data[0]
        segmentNumber=bytes(reversed(await self.recvBytesExactLen(3)))
        segmentId=str(segmentNumber.hex())
        print("* Requested Segment ID: " + segmentId)
        print("* Requested PacketNumber: " + str(packetNumber))
        if segmentId != "7fffff" and segmentId != loadedpak:
            loadpak(segmentId)
            await self.sendBytes(bytes([0x91]))
        else:
            if segmentId == "7fffff":
                print("Time packet requested")
                await self.sendBytes(bytes([0xe4, 0x91]))
                segmentId == ""
                response = await self.recvBytesExactLen(2)
                print("* Response from NPC: " + response.hex(" "))
                print("segmentId", segmentId)
                print("packetnumber", packetNumber)
                print("segments", segments)
                await self.send_time()
                await self.sendBytes(bytes([0x10, 0xe1]))

            else:
                await self.sendBytes(bytes([0xe4, 0x91]))
                response = await self.recvBytesExactLen(2)
                print("* Response from NPC: " + response.hex(" "))
                print("segmentId", segmentId)
                print("packetnumber", packetNumber)
                print("segments", segments)

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
                escaped_pack_data = self.escapeUploadBytes(pack_data)
                await self.sendBytes(escaped_pack_data)
                await self.sendBytes(bytes([0x10, 0xe1]))

    async def handle_set_channel_code(self, data):
        global channelCode
        await self.send_ack()
        data = await self.recvBytesExactLen(2)
        while len(data) < 2:
            remaining = 2 - len(data)
            print("Waiting for channel code")
            print(data.hex(' '))
            if(remaining > 0):
                data = data + await self.recvBytesExactLen(remaining)

        print("* Received Channel code bytes: " + data.hex())
        channelCode = bytes(reversed(data)).hex()
        print("* Channel code: " + channelCode)
        await self.sendBytes(bytes([0xe4]))

    async def handle_0x8f_req(self, data):
        print("* 0x8f request")
        data = await self.recvBytes()
        await self.sendBytes(bytes([0xe4]))

    def handle_unimplemented_req(self, data):
        print("* ??? Unimplemented request")
        print("* " + data.hex(' '))

    def escapeUploadBytes(self, data):
        escapedBytes = bytearray()

        for idx in range(len(data)):
            byte=data[idx]
            if(byte == 0x10):
                escapedBytes.append(byte)
                escapedBytes.append(byte)
            else:
                escapedBytes.append(byte)

        return escapedBytes

    async def sendBytes(self, data):
        chunk_size=2000
        index=0
        end=len(data)

        while index + chunk_size < end:
            self.writer.write(data[index:index+chunk_size])
            print("NA-->NPC:  " + data[index:index+chunk_size].hex(' '))
            index += chunk_size

        if index != end:
            print("NA-->NPC:  " + data[index:end].hex(' '))
            self.writer.write(data[index:end])

        # await self.writer.drain()
        # print("Drained.")


    async def recvBytesExactLen(self, length=None):
        if(length is None):
            return None
        data = await self.recvBytes(length)

        while len(data) < length:
            remaining = length - len(data)
            print("Waiting for {} more bytes".format(length - len(data)))
            print(data.hex(' '))
        #     time.sleep(0.01)
            data = data + await self.recvBytes(remaining)
        return data


    async def recvBytes(self, length = None):
        if(length is None):
            length = MAX_READ
        data = await self.reader.read(length)

        if(len(data) > 0):
            print("NPC-->NA:   " + data.hex(' '))
        
        return data

# Loads pak from file, assumes file names are all upper case with a lower case .pak extension
# Assumes all pak files are in a directory called paks/
# 000001.pak from cycle 2 needs to have the last 16 bytes of 1A's stripped off
# 000001.pak from cycle 1 is horribly corrupt and dosen't work

def loadpak(filename):
    file = filename.upper()
    global segments 
    segments = {}
    print("* Loading NABU Segments into memory")
    global segment1
    segment1 = NabuSegment()
    segment1.ingest_from_file( "paks/"+ file + ".pak")
    segments[filename] = segment1
    global loadedpak
    loadedpak = filename

######  Begin main code here

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
# ser = serial.Serial(port=args.ttyname, baudrate=args.baudrate, timeout=0.5, stopbits=serial.STOPBITS_TWO)
segments = {}

print("* Loading NABU Segments into memory")
loadpak("000001")

# Crude implementation - we could probably scan a directory for files here.
# TODO: The PAK file is pre-split into individual packets, each with headers and checksums. 
# TODO: We should change this to handle .nabu files instead, which have not yet been split into packets with headers and checksums
# 


async def handle_connection(reader, writer):
    nabu_session = NabuAdaptor(reader,writer)
    await nabu_session.run_NabuSession()

async def main():

    server = await asyncio.start_server(
            handle_connection, '0.0.0.0', 5816)
    addrs = ', '.join(str(sock.getsockname()) for sock in server.sockets)
    print(f'Serving on {addrs}')

    async with server:
        await server.serve_forever()

print ("Started...")
asyncio.run(main())

