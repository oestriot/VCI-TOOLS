import struct
import sys
import os


def extractvci(vcifile):
    r = open(vcifile, "rb")
    vci_header = r.read(0x200)
    mbr = r.read(0x50)

    partitons = []

    typetoname = {0x9: "gro0", 0xa: "grw0", 0xd: "mediaid"}

    for i in range(0, 0x10):
        partition = struct.unpack("IIBBBIH", r.read(0x11) + b"\x00")
        off = partition[0]
        sz = partition[1]
        cde = partition[2]
        typ = partition[3]
        acti = partition[4]
        flgs = partition[5]
        unk = partition[6]
        
        
        if cde == 0x00:
            continue
        elif cde == 0x9:
            print("-- gro0")
        elif cde == 0xa:
            print("-- grw0")
        elif cde == 0xd:
            print("-- mediaid")

        partitons.append({"off": off, "sz": sz, "code": cde, "type": typ, "flags": flgs})
        
        print("off: "+hex(off))
        print("sz: "+hex(sz))
        print("code: "+hex(cde))
        print("type: "+hex(typ))
        print("active: "+hex(acti))
        print("flags: "+hex(flgs))
        print("unk: "+hex(unk))
        
        
        
        print("\n\n\n")
        
    for partition in partitons:
        r.seek((partition["off"] + 1) * 0x200, os.SEEK_SET)
        partname = typetoname[partition["code"]]
        print("writing: "+partname)
        open(vcifile+"-"+partname+".img", "wb").write(r.read(partition["sz"] * 0x200))

    print("writing: keys")
    r.seek(0x10, os.SEEK_SET)
    open(vcifile+"-keys.bin", "wb").write(r.read(0x40))

    r.close()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: <vcifile>")
        quit()
        
    vcifile = sys.argv[1]
    extractvci(vcifile)