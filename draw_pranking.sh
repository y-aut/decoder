#!/bin/bash

# args: filename, params...

# prank の結果を pimage に渡して実行
res=$(./release.exe prank "$@")
echo "$res"
res=($res)
size=${#res[*]}
param=""
for ((i = 0; i < $size; i += 3)); do
    param+="-l ${res[$i]} ${res[$((i+1))]} "
done
./release.exe pimage $param $1 -r
convert out.bmp out.png
