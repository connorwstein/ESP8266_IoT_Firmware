#!/bin/sh

PORT=$1

if [ -z "$PORT" ]
then
	PORT=/dev/ttyUSB0
fi

make clean
make -f Makefile2

until sudo -E env "PATH=$PATH" make flash ESPPORT=$PORT; do
	sleep 1
done

sudo screen $PORT 115200
