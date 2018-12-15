#!/bin/bash
cd Git/tp-2018-2c-Los-5digos/CPU/Debug
make clean
make all
./CPU ../config.cfg
exec bash
