#!/bin/sh

FISH_LIST="RACHEL ROSIE RONIN GERTRUDE GREG GORDON YAMES YASMIN YOLANDA"

rm -rvf ./build
mkdir ./build

for fish in $FISH_LIST
do
    echo
    echo "################"
    echo "catching $fish"
    echo "################"

    echo "#define $fish" > ./src/fish.h &&
    pio run -e lolin32 &&
    cp -v ./.pioenvs/lolin32/firmware.bin ./build/${fish}.bin
done
