#!/usr/bin/env python3

import os
from pcapng import FileScanner
from pcapng.blocks import EnhancedPacket

class MatchSingleDefinition:
    def __init__(self, offset, data):
        self.offset = offset
        self.data = data

class MatchDefinition:
    def __init__(self, label: str, matches: list[MatchSingleDefinition]):
        self.label = label
        self.matches = matches

    def match_single(self, inp: bytes, match: MatchSingleDefinition):
        return inp[match.offset:match.offset + len(match.data)] == match.data

    def match(self, inp: bytes):
        for match in self.matches:
            if not self.match_single(inp, match):
                return False
        return True

def isolateInterestingHidData(data: bytes) -> bytes:
    for k in range(len(data) - 1, -1, -1):
        if data[k] != 0:
            return data[:k + 1]
    # All zeroes I guess
    return b''

def hexdump(prefix_len: int, data: bytes) -> str:
    ret = '"'
    k = 0
    while k < len(data):
        ret += f"\\x{data[k]:02x}"
        k += 1
        if (k % 16) == 0:
            ret += f"\"\n"
        if k == len(data):
            break
        if (k % 16) == 0:
            ret += f"{' '*prefix_len}\""
    if k % 16 != 0:
        ret += "\""
    return ret

testFile = os.environ.get('TESTFILE', os.path.join(os.path.dirname(__file__), 'data/m65pro-tilt-controls-2.pcapng'))
matchInput = MatchDefinition('input', [MatchSingleDefinition(0x11, b'\x01\x00\x02\x00\x84')])
matchOutput = MatchDefinition('output', [MatchSingleDefinition(0x11, b'\x01\x00\x02\x00\x04')])
matchIface3 = MatchDefinition('iface3', [MatchSingleDefinition(0x11, b'\x01\x00\x02\x00\x03')])
hidStuffOffset = 0x1b # the URB header appears to extend to this offset, reliably

matchQuery = MatchDefinition('query', [MatchSingleDefinition(0x1, b'\x02')])
ledControl = MatchDefinition('led', [MatchSingleDefinition(0, b'\x09\x06\x00\x06')])

counter = 0
cmdList = []
print ("#include <stdint.h>")
print ("#include <stdlib.h>")
print ("#include \"stuff.h\"")
with open(testFile, 'rb') as porky:
    scanner = FileScanner(porky)
    for block in scanner:
        if isinstance(block, EnhancedPacket):
            counter += 1
            data = block.packet_data[:block.packet_len] # in case the input buffer is sized stupidly
            if len(data) == hidStuffOffset:
                continue # ignored
            if matchInput.match(data):
                stuff = isolateInterestingHidData(data[hidStuffOffset:])
#                print (f"{counter:7d}: len {block.packet_len}: {hexdump('input', stuff).strip()}")
            elif matchOutput.match(data):
                stuff = isolateInterestingHidData(data[hidStuffOffset:])
                if matchQuery.match(stuff) or ledControl.match(stuff): # We're not interested
                    continue
                name = f"cmd{counter:05d}"
                print (f"static uint8_t {name}[PACKET_SIZE] = {hexdump(20, stuff)};")
                cmdList.append(name)

print ("uint8_t* const replaySequence[] = {")
for cmd in cmdList:
    print (f"    {cmd},")
print ("    NULL\n};")
