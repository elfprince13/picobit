#!/usr/bin/env python3

def hex_to_sparse_bin(hexData):
    print(hexData)
    upper_nibbles = hexData[::2]
    lower_nibbles = hexData[1::2]
    print(upper_nibbles)
    print(lower_nibbles)
    return bytearray(int(upper_nibble + lower_nibble,16) for upper_nibble, lower_nibble in zip(upper_nibbles,lower_nibbles))

if __name__ == "__main__":
    import sys
    with open(sys.argv[1], 'r') as infile:
        hexData = infile.readlines()
    #binData = hex_to_sparse_bin(hexData.replace(":", ""))
    with open(sys.argv[2], 'wb') as outfile:
        outfile.write(b'\xfb\xd7')
        for hexLine in hexData:
            hexLine = hexLine.strip().replace(":", "")
            if hexLine:
                binLine = hex_to_sparse_bin(hexLine)
                outfile.write(binLine)