#!/bin/sh
PAK_DIR=~/code/nabu-homebrew/compiled-pak

zcc +nabu -create-app -lndos -compiler sdcc -SO3 -DAMALLOC -o LIFF.bin life.c tms9918.c

ls -l
#	mv 000001_code_compiler.bin 000001.nabu && 
mv 000001.nabu $PAK_DIR
