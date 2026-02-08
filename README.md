# VCI-TOOLS

tools to convert .vci, psv, and other vita disk images to other formats, 

# Overview:

## extractgc 

extracts all files from all partitions on an image (eg: .vci, .psv, .img) 
or can also extract the exFAT partition on them;

```
Usage: extractgc <gc_image> [dump_raw] [out_file]
gc_image        path to a .img, .psv, or .vci file.
dump_raw        'true' or 'false'       determines wether to extract disk images, or extract filesystems
output_folder	path to save the output files to
```

note: this can also be used to extract raw eMMC or Memory card dumps (or anything using SceMBR)

## gc2nonpdrm

creates a NoNpDrm backup from a gamecart image (vci or psv) 
```
Usage: gc2nonpdrm <vci or psv> [output_folder]
psv_or_img_file	input file to convert
output_folder	output nonpdrm format game (optional)
```
## gc2vci

converts a gc image of format (.img, .psv) to .vci format; 
doing so requires that you have dumped the p20/p18 keys for said cartridge 

NOTE: these keys are cart-specific, if you have keys from another cart it wont work!

```
Usage: gc2vci <psv_or_img_file> <keys_file> [vci_file]
psv_or_img_file	input file to convert
keys_file		p18/p20 key file
vci_file		output vci file (optional)
```

## gc2img
converts a gc image (.vci, .psv) to a raw SceMBR image; 

```
Usage: vci2psv <gc_img_file> [img_file]
gc_img_file	file to convert
img_file	output img file (optional)
```

## vci2psv
converts a gctoolkit (.VCI) image to (psvgamesd) .psv format;

```
Usage: vci2psv <vci_file> [psv_file] [keys_file]
vci_file	file to convert
psv_file	output psv file (optional)
keys_file	output p18/p20 key file (optional)
```

