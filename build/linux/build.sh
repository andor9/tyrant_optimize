#!/bin/bash

set -xue
#$TRAVIS_TAG-build-$TRAVIS_BUILD_NUMBER
#cmake . -Bbuild-dir -DVERSION:STRING="${TRAVIS_TAG}-build-${TRAVIS_BUILD_NUMBER}"
#cmake --build build-dir

make test

set +xue
