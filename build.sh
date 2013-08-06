#!/bin/bash

cd AIOUSB

source sourceme.sh

export CPATH=/usr/include/libusb-1.0

cd $AIO_LIB_DIR
make
cd $AIO_CLASSLIB_DIR
make

