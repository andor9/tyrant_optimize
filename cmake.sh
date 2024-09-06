#!/bin/bash
export TRAVIS_TAG=TMPDebug
export TUO_PLATFORM=windowsXX
export TUO_TARGET=windows
./build/${TUO_TARGET}/build.sh
