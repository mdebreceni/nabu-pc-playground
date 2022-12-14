#!/usr/bin/env python3

# A YUNN program (called segment) is a single file of concatenated binaries called packs:
#
# segment
#         +-------+
#         | pak_0 |
#         +-------+
#         | pak_1 |
#         +-------+
#         .........
#         +-------+
#         | pak_n |
#         +-------+
#         
# Each pack is no more than 1KB and has this structure
# 
# pack
#         +----------------+
#         | header 18 bytes|
#         +----------------+
#         | code =<1004    |
#         | bytes          |
#         +----------------+
#         | CRC  2 bytes   |
#         +----------------+
#         
# The meaning of header bytes is as follows:
#         bytes 0,1:     pack length (excluding the header and CRC bytes, lsb first)
#         bytes 2,3,4:   segment id (msb firs)
#         byte 5:        pack id (with the first id = 0)
#         byte 6:        segment owner (the same for all PAKs; in UNN segment owner=1)
#         bytes 7-10:    segment tier  (the same for all PAKs; in YUNN, tier= $08 $00 $00 $02)
#         bytes 11,12:   mystery bytes (the same for all PAKs; in YUNN, these bytes are $80 $19)
#         byte  13:      paktype
#         byte  14,15:   pack number (the same as byte 5)
#         byte  16,17:   offset -- pointer to the beginning of the pack from the beginning of the segment
# 
# Segments for YUNN are created by special software which takes, as its input, a Z80-assembled data, directory, 
# overlay, or code binary and outputs a .PAK segment.

class NabuSegment:
    def __init__(self):
        self.packs = {}

    def load_pack(self, pack_bytes, pack_id):
        self.packs[pack_id] = pack_bytes

    def get_pack(self, pack_id):
        print("* get_pack(pack_id = {})".format(pack_id))
        if pack_id in self.packs:
            print("* Found!")
            return self.packs[pack_id]
        else:
            print("* Not found :(")
            return None

    def get_pack_count(self):
        return len(self.packs)

    def parse_segment(self, segment_bytes):
        index = 0
        while index < len(segment_bytes):
            pack_length = segment_bytes[index] + segment_bytes[index + 1] * 256
            pack_id = segment_bytes[index + 5]

            print("* Pack length is {}".format(pack_length))
            print("* Pack ID is {}".format(pack_id))
            index += 2
            pack_end = index + pack_length
            print("* Index = {}.  Pack_end = {}.".format(index, pack_end))
            pack_bytes = segment_bytes[index:pack_end]
            print("* Pack bytes: {}".format(pack_bytes.hex(' ')))

            self.packs[pack_id] = pack_bytes
            index += pack_length

    def ingest_from_file(self, pakfile):
        f = open(pakfile, "rb")
        contents = bytes(f.read())
        print(" * Ingesting Packs from {}:".format(pakfile) + contents.hex(' '))
        self.parse_segment(contents)

class NabuPack:
    def __init__(self):
        pack_segment_id = None
        pack_packnum = None
        segment_owner = None
        setment_tier = None
        segment_mystery_bytes = None
        pack_type = None
        pack_number = None
        pack_offset = None
        pack_crc = None
        pack_bytes = None

    def ingest_bytes(self, pack_bytes):
        self.pack_segment_id = bytes(pack_bytes[0:3])
        self.pack_packnum = bytes(pack_bytes[3:4])
        self.segment_owner = bytes(pack_bytes[4:5])
        self.segment_tier = bytes(pack_bytes[5:9])
        self.segment_mystery_bytes = bytes(pack_bytes[9:11])
        self.pack_type = bytes(pack_bytes[11:12])
        self.pack_number = bytes(pack_bytes[12:14])
        self.pack_offset = bytes(pack_bytes[14:16])
        self.pack_crc = bytes(pack_bytes[-2:])
        self.pack_bytes = pack_bytes

