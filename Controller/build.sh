#!/bin/bash
CC="avr-gcc"
SRC="main.c"
SRC+=" buffers.c"
SRC+=" button.c"
SRC+=" commands.c"
SRC+=" events.c"
SRC+=" gpio.c"
SRC+=" log.c"
SRC+=" serial_console.c"
SRC+=" serial.c"
SRC+=" timer.c"
SRC+=" input_driver.c"
DEF="-D__AVR_ATmega328P__"
OPTS="-mmcu=atmega328p -Os -Wall -Waddr-space-convert"
OUT="controller"

# Compile into build directory
cd src
${CC} ${OPTS} ${DEF} -o ../build/${OUT}.elf ${SRC}
cd ..

# Post-build steps
avr-objcopy -j .text -j .data -O ihex build/${OUT}.elf build/${OUT}.hex
