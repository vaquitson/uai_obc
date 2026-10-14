#!/bin/sh

rm -rf build

make SIMULATION=native prep
make
