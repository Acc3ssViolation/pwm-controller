#!/bin/bash

OUT="controller"
HEX="build/${OUT}.hex"

avrdude -v -p atmega328p -c arduino -P /dev/ttyACM0 -b 115200 -U flash:w:build/${OUT}.hex:i