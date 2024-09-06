#!/bin/bash

set -xue


#DEPLOY_FILE=tuo_@VERSION@_${TUO_PLATFORM}.exe

#cp build-dir/tuo $TUO_FILE

export TUO_FILE_ZIP="tuo.${TRAVIS_TAG}_64_bit_osx.zip" 

#ls -la

zip -r "$TUO_FILE_ZIP" data/ *.osx


set +xue
