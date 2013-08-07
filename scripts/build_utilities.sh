#!/bin/bash


function build_cmd {
    echo "# $(date '+<%m/%d/%Y %H:%M:%S>') $@" | tee -a ${BUILD_LOG};
    eval $@ 2>&1 | tee -a ${BUILD_LOG};
}

function build_cd {
    echo "# $(date '+<%m/%d/%Y %H:%M:%S>') cd $@" | tee -a ${BUILD_LOG};
    cd $@
}


function start_build {
    echo "# $(date '+<%m/%d/%Y %H:%M:%S>')  Starting build" | tee -a ${BUILD_LOG}
}
