#!/bin/sh

make clean
make -f Makefile2

until sudo -E env "PATH=$PATH" make flash; do
	sleep 1
done

screen /dev/ttyUSB0 115200