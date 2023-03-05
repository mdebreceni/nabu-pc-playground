#!/bin/sh
PAK_DIR=~/code/nabu-homebrew/compiled-pak

# zcc +z80 -mz80 -startup 0 -zorg 0x140D --no-crt -lm Life.c -o 000001.nabu && 
#	mv 000001_code_compiler.bin 000001.nabu && 
#	mv 000001.nabu $PAK_DIR

zcc +nabu -mz80 -compiler sdcc -O2 -o 000001.nabu tms9918.c Life.c &&
	mv 000001_code_compiler.bin 000001.nabu && 
	mv 000001.nabu $PAK_DIR
