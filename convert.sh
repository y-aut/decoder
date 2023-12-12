#!/bin/bash

# bmp を png に変換
for file in *.bmp; do
    name=$(basename $file .bmp)
    convert $name.bmp $name.png
    echo Converted $name.bmp to $name.png
done
