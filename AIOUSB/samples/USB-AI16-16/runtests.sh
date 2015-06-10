#!/bin/bash

function dynmake() {
    make $1 CFLAGS=" -I${AIO_LIB_DIR} " LDFLAGS="-L${AIO_LIB_DIR}" "$@" DEBUG=1
}

dynmake burst_test
dynmake continuous_mode
dynmake read_channels_test 
dynmake bulk_acquire_sample                                                                                               


./burst_test -V --buffersize 20000 --range 0-5=2 -c 20000 -D 31
result=$(r -e 'tmp<-read.csv("output.txt",header=F); cat(if(max(tmp$V1) > 60000) { "ok" } else { "not ok" } )')
if [ $result == "not ok" ] ; then
    echo "ERROR: max value not ok"
    exit 1
fi
result=$(r -e 'tmp<-read.csv("output.txt",header=F); cat(if(min(tmp$V1) < 200) { "ok" } else { "not ok" } )')
if [ $result == "not ok" ] ; then
    echo "ERROR: min value not ok"
    exit 1
fi

./continuous_mode  -V --num_scans 100000 --range 0-4=0 -c 20000 -D 31

result=$(r -e 'tmp<-read.csv("output.txt",header=F); cat(if(max(tmp$V1) > 4.6) { "ok" } else { "not ok" } )')
if [ $result == "not ok" ] ; then
    echo "ERROR: max value not ok"
    exit 1
fi
result=$(r -e 'tmp<-read.csv("output.txt",header=F); cat(if(min(tmp$V1) < 0.3) { "ok" } else { "not ok" } )')
if [ $result == "not ok" ] ; then
    echo "ERROR: min value not ok"
    exit 1
fi

bulk_acquire_sample
if [ "$?" != "0" ] ; then
    echo "Bulk acquire error"
    exit 1
fi

read_channels_test -c 1000 > output.txt
perl -i -ne 'if ( s/^(\d+.*)$/$1/g ) { print; }' output.txt
result=$(r -e 'tmp<-read.csv("output.txt",header=F); cat(if(max(tmp$V2) > 4.6) { "ok" } else { "not ok" } )')
if [ $result == "not ok" ] ; then
    echo "ERROR: max value not ok"
    exit 1
fi

result=$(r -e 'tmp<-read.csv("output.txt",header=F); cat(if(min(tmp$V2) < 0.3) { "ok" } else { "not ok" } )')
if [ $result == "not ok" ] ; then
    echo "ERROR: min value not ok"
    exit 1
fi


