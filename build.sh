#!/bin/bash 


rm -f ndvm
gcc -o ndvm ndvm.c -lpthread -lncurses -g
./ndvm
