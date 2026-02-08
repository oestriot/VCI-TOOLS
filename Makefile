CC = gcc
OUTDIR = build
CFLAGS = -O3 -fPIC 
OBJ = lib/aes.o lib/exfat.o lib/gcauthmgr.o lib/hmac_sha256.o \
	  lib/mbr.o lib/npdrm.o lib/path.o lib/sha1.o lib/sha256.o \
	  lib/ff/ff.o lib/ff/ffsystem.o lib/ff/ffunicode.o lib/ff/diskio.o
  
ifeq ($(OS),Windows_NT)
	DELCMD := rd /S /Q
	OBJ += lib/compat.o
else
	DELCMD := rm -rf
endif

all: build extractgc gc2img gc2nonpdrm gc2vci vci2psv

.PHONY: build
build:
	-mkdir $(OUTDIR)
	
.PHONY: clean
clean:
	-$(DELCMD) "*.o"
	-$(DELCMD) "lib/*.o"
	-$(DELCMD) "lib/ff/*.o"
	-$(DELCMD) $(OUTDIR)


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

extractgc: $(OBJ) extractgc.o
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS)

gc2img: $(OBJ) gc2img.o
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS)

gc2nonpdrm: $(OBJ) gc2nonpdrm.o
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS)

gc2vci: $(OBJ) gc2vci.o
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS)
	
vci2psv: $(OBJ) vci2psv.o
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS)

