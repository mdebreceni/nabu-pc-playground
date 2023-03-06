#!/bin/sh
PAK_DIR=~/code/nabu-homebrew/compiled-pak

zcc +nabu -create-app -lndos -compiler sdcc -SO3 -DAMALLOC -o LIFE.bin Life.c tms9918.c
mv LIFE.NABU $PAK_DIR/000001.nabu
