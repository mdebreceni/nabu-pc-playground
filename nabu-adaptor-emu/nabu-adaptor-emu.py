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
from nabu_pak import NabuSegment, NabuPack

from twisted.internet.protocol import Factory
from twisted.internet.endpoints import TCP4ServerEndpoint
from twisted.protocols.basic import LineReceiver
from twisted.internet import defer
from twisted.internet.defer import inlineCallbacks, returnValue
from twisted.internet import reactor


NABU_STATE_AWAITING_REQ = 0
NABU_STATE_PROCESSING_REQ = 1
MAX_READ=65535

class NabuAdaptorFactory(Factory):
    def __init__(self):
        print("NabuAdaptorFactory::__init__")
        read_limit = MAX_READ
        return

    def buildProtocol(self, addr):
        print("NabuAdaptorFactory::buildProtocol()")
        return NabuAdaptor()

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

class NabuAdaptor(LineReceiver):
    def __init__(self):
        print("Called NabuAdaptor::__init__")
        self.setRawMode()
        self.read_limit = MAX_READ
        self.d = None
        self.read_buffer = bytearray(b'')
        self.state = NABU_STATE_AWAITING_REQ

    def connectionMade(self):
        print("Got a connection. Yay!")

    def connectionLost(self, reason):
        print("Lost connection. Boo!")

    def readBuffered(self):
        print("readBuffered")
        read_len = len(self.read_buffer)
        print("readBuffered: read_len is {}".format(read_len))
        data = None
        if read_len > self.read_limit:
            read_len = self.read_limit
            data = self.read_buffer[0:read_len]
            self.read_buffer = self.read_buffer[read_len:]
        else:
            data = self.read_buffer
            self.read_buffer = bytearray(b'')
        return data

    def rawDataReceived(self, data):
        print(type(data))
        print("(rawDataReceived) NPC-->NA:   " + data.hex(' '))

        if self.state == NABU_STATE_PROCESSING_REQ:
            print("State: NABU_STATE_PROCESSING_REQ")
            self.read_buffer += data
            print("readBuffer" + self.read_buffer.hex(' '))
            if(self.d is not None):
                print("Yay - we have a callback")
                cb_data = self.readBuffered()
                self.d.callback(cb_data)
                self.d = None
            else:
                print("ZBooo.  no callback")
        else:
            print("State: NABU_STATE_AWAITING_REQ - just got REQ")
            self.state=NABU_STATE_PROCESSING_REQ
            self.handle_NabuRequest(data)


    def handle_NabuRequest(self, data):
        if len(data) > 0:
            req_type = data[0]
            if req_type == 0x03:
                print("* 0x03 request")
                self.handle_0x03_request(data)
            elif req_type == 0x0f:
                print("* 0x0f request")
                self.handle_0x0f_request(data)
            elif req_type == 0xf0:
                print("* 0xf0 request")
                self.andle_0xf0_request(data)
            elif req_type == 0x80:
                print("* Reset segment handler")
                self.handle_reset_segment_handler(data)
            elif req_type == 0x81:
                print("* Reset")
                self.handle_reset(data)
            elif req_type == 0x82:
                print("* Get Status")
                self.handle_get_status(data)
            elif req_type == 0x83:
                print("* Set Status")
                self.handle_set_status(data)
            elif req_type == 0x84:
                print("* Download Segment Request")
                self.handle_download_segment(data)
            elif req_type == 0x85:
                print("* Set Channel Code")
                self.handle_set_channel_code(data)
                print("* Channel code is now " + channelCode)
            elif req_type == 0x8f:
                print("* Handle 0x8f")
                self.handle_0x8f_req(data)
            elif req_type == 0x10:
                print("got request type 10, sending time")
                self.send_time()
                self.sendBytes(bytes([0x10, 0xe1]))
    
            else:
                print("* Req type {} is Unimplemented :(".format(data[0]))
                self.handle_unimplemented_req(data)

        self.state=NABU_STATE_AWAITING_REQ
    
    def send_ack(self):
        self.sendBytes(bytes([0x10, 0x06]))

    # Pre-formed time packet, sends Jan 1 1984 at 00:00:00
    # Todo - add proper time packet generation and CRC 

    def send_time(self):
        self.sendBytes(bytes([0x7f, 0xff, 0xff, 0x00, 0x01, 0x7f, 0xff, 0xff, 0xff, 0x7f, 0x80, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x54, 0x01, 0x01, 0x00, 0x00, 0x00, 0xc6, 0x3a]))

    # TODO:  We can probably get rid of handle_0xf0_request, handle_0x0f_request and handle_0x03_request
    # TODO:  as these bytes may have been from RS-422 buffer overruns / other errors

    def handle_0xf0_request(self, data):  
        self.sendBytes(bytes([0xe4]))

    def handle_0x0f_request(self, data):
        self.sendBytes(bytes([0xe4]))

    def handle_0x03_request(self, data):
        self.sendBytes(bytes([0xe4]))

    def handle_reset_segment_handler(self, data):
        self.sendBytes(bytes([0x10, 0x06, 0xe4]))

    def handle_reset(self, data):
        self.send_ack()

    def handle_get_status(self, data):
        global channelCode
        self.send_ack()
        response = self.recvBytes()
        print("Response is of type {}".format(type(response)))
        print("Len(response) = {}".format(len(response)))
        print("Sent ack")
        if channelCode is None:
            print("* Channel Code is not set yet.")
            # Ask NPC to set channel code
            self.sendBytes(bytes([0x9f, 0x10, 0xe1]))
        else:
            print("* Channel code is set to " + channelCode)
            # Report that channel code is already set
            self.sendBytes(bytes([0x1f, 0x10, 0xe1]))

    def handle_set_status(self, data):
        self.sendBytes(bytes([0x10, 0x06, 0xe4]))

    def handle_download_segment(self, data):
        # ; segment load request
        # [11]        NPC       $84
        #              NA        $10 06
        self.send_ack()
        packetNumber=self.recvBytesExactLen(1)[0]
        segmentNumber=bytes(reversed(self.recvBytesExactLen(3)))
        segmentId=str(segmentNumber.hex())
        print("* Requested Segment ID: " + segmentId)
        print("* Requested PacketNumber: " + str(packetNumber))
        if segmentId != "7fffff" and segmentId != loadedpak:
            loadpak(segmentId)
            self.sendBytes(bytes([0x91]))
        else:
            if segmentId == "7fffff":
                print("Time packet requested")
                self.sendBytes(bytes([0xe4, 0x91]))
                segmentId == ""
            else:
                self.sendBytes(bytes([0xe4, 0x91]))

                response = self.recvBytes(2)
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
                self.sendBytes(escaped_pack_data)
                self.sendBytes(bytes([0x10, 0xe1]))

    def handle_set_channel_code(self, data):
        global channelCode
        self.send_ack()
        data = self.recvBytesExactLen(2)
        while len(data) < 2:
            remaining = 2 - len(data)
            print("Waiting for channel code")
            print(data.hex(' '))
            data = data + self.recvBytes(remaining)

        print("* Received Channel code bytes: " + data.hex())
        channelCode = bytes(reversed(data)).hex()
        print("* Channel code: " + channelCode)
        self.sendBytes(bytes([0xe4]))

    def handle_0x8f_req(self, data):
        print("* 0x8f request")
        data = self.recvBytes()
        self.sendBytes(bytes([0xe4]))

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

    def sendBytes(self, data):
        chunk_size=6
        index=0
        delay_secs=0
        end=len(data)

        while index + chunk_size < end:
            self.transport.write(data[index:index+chunk_size])
            print("NA-->NPC:  " + data[index:index+chunk_size].hex(' '))
            index += chunk_size
            time.sleep(delay_secs)

        if index != end:
            print("NA-->NPC:  " + data[index:end].hex(' '))
            self.transport.write(data[index:end])

    def recvBytesExactLen(self, length=None):
        if(length is None):
            return None
        data = self.recvBytes(length)

        while len(data) < length:
            remaining = length - len(data)
            print("Waiting for {} more bytes".format(length - len(data)))
            print(data.hex(' '))
            time.sleep(0.01)
            data = data + self.recvBytes(remaining)
        return data

#    def awaitBytes(self, length):
#        self.read_limit = length;
#        self.d = defer.Deferred()
#        print("in awaitBytes(): self.d has type {}".format(type(self.d)))
#        return self.d

    def get_cb_data(self, cb_data):
        print("get_cb_data was called!")
        return cb_data


    @defer.inlineCallbacks
    def readAsyncUsingYield(self, length):
        print("readAsyncUsingYield")
        self.d = twisted.internet.defer.Deferred()
        self.read_limit = length
        # defer.Deferred()
        print("self.d is of type {}".format(self.d))
        self.d.addCallback(self.get_cb_data())
        print("self.d.added_callback()")
        data = yield self.d

        defer.returnValue(data)


    def recvBytes(self, length = None):
        if(length is None):
            length = MAX_READ
        if(len(self.read_buffer) > 0):
            print("Reading synchronously (length = {})".format(length))
            self.read_limit = length
            data = self.readBuffered()
        else:
            data = yield self.readAsyncUsingYield(length)
            print("Data has type {}".format(type(data)))
            print("returnValue happened")

        if(len(data) > 0):
            print("NPC-->NA:   " + data.hex(' '))
        
        return data

    def readAsync(self, length):
        self.read_limit = length



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

#while True:
#    data = recvBytes()
print ("Start... the reactor")
reactor.listenTCP(5816, NabuAdaptorFactory())
reactor.run()

print ("Started...")
