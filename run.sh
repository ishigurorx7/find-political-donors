#!/bin/bash

make -f Makefile
arg="-f ./input/itcont.txt -o ./output/medianval_by_zip.txt -m 0"
./find_political_donors  $arg






