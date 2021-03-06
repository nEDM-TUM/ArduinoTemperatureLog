#!/bin/bash

CONFIG_FILE="SETTINGS"

cp ./code.ino ./src/temp.ino

while read key val; do
	sed -i "s/^#define $key.*$/#define $key $val/" "./src/temp.ino"
done < $CONFIG_FILE

ino clean
ino build -m ethernet
ino upload -m ethernet
ino clean

rm ./src/temp.ino
