#!/bin/bash
# flashes the game in PROD mode
PLATFORMIO_SRC_DIR=$(pwd)/src/kiste-1 pio run --target=upload --environment=nanoatmega328
