#!/bin/sh

clear
gcc -Wall -std=gnu99 -g -o btrade *.c \
$(pkg-config libcurl --libs --cflags) \
$(pkg-config libssl --libs --cflags) \
-lpthread \
-ljson -Wl,--rpath=/usr/local/lib

### FÜR DEBUGGING AUSKOMMENTIEREN
strip ./btrade

# EOF

