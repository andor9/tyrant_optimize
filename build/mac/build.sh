#!/bin/bash

set -xue

make
make debug

chmod 755 tuo.osx

./tuo.osx -version


#cp tuo.osx $TUO_FILE_32

#cp tuo.osx $TUO_FILE_64

set +xue
