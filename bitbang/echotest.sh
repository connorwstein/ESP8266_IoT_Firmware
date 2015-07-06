#!/bin/bash

while true; do 
sleep 1 
sudo echo -e -n "\x00" >> /dev/ttyUSB0
done

