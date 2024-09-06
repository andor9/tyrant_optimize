#!/bin/bash

set -xue

sudo apt-get update

sudo apt-get --yes install \
    g++ make cmake \
    libboost-all-dev \

set +xue
