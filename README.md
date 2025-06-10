# VCI-TOOLS

python scripts to convert .vci images to other formats, 

tools overview:

# dependancies

the tools here require FATtools and pycryptodome libraries,
and are expected to run using Python3 not Python2.

install with:
```
pip3 install FATtools pycryptodome
```

# vci2psv 
converts a .vci (gctoolkit) image into a .psv image (psvgamesd)
where psvfile is where to output the .psv; optionally can also 
extract p20/p18 keys to a file.

usage is: 
```
vci2psv.py <vcifile> [psvfile] [keyfile]
```

# psv2vci 
given both .vci (gctoolkit) and .psv (psvgamesd) store a raw image of the GC
the only difference is one doesn't have packet20/packet18 keys;

this script can turn a .psv file along with a external copy of p20/p18 keys 
(from i.e "backup gamecart keys" option in GCToolKit); 
to build a .vci file;

usage is:
```
psv2vci.py <psvfile> <keyfile> [vcifile]
```

# extractvci

this can parse out and extract the SceMBR of a .vci dump;
providing raw disk images of gro0, grw0, mediaid, and also p20/p18 keyfiles;

usage is:
```
extractvci.py <vcifile>
```

# gc2nonpdrm

this can extract all the files from a gamecart image (either .vci or .psv) 
and create a functional NoNpDrm dump of the game, including a fake rif / work.bin;

usage is
```
gc2nonpdrm.py <vcifile or psvfile>
```
