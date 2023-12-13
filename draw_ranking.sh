#!/bin/bash

# args: filename, params...

# rank の結果を image に渡して実行
res=($(./release.exe rank "$@"))
size=${#res[*]}
param=""
for ((i = 0; i < $size; i += 3)); do
    param+="-l ${res[$i]} ${res[$((i+1))]} "
done
./release.exe image $param $1
convert out.bmp out.png
