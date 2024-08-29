#!/bin/bash

OUT="controller"
HEX="build/${OUT}.hex"

# Arduino UNO
# avrdude -v -p atmega328p -c arduino -P /dev/ttyACM0 -b 115200 -U flash:w:build/${OUT}.hex:i

# Arduino as ISP
avrdude -v -p atmega328p -c stk500v1 -P /dev/ttyACM0 -b 19200 -U flash:w:build/${OUT}.hex:i