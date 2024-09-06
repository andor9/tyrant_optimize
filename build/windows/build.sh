#!/bin/bash

set -xue

#Linux Build
#make all
MXE_DIR=/usr/lib/mxe


declare -a a_openmp=("" "-openmp" )
declare -a a_debug=("" "-debug" "-time")
declare -a a_bit=("" "-x86" )

for omp in "${a_openmp[@]}"
do
for dbg in "${a_debug[@]}"
do
for bit in "${a_bit[@]}"
do

DFLAGS=""
BDIR="build-dir${bit}${omp}${dbg}"
NAME="tuo${bit}${omp}${dbg}.exe"

if [ "$bit" = "" ]; then
#Windows x64 Build
MXE_TARGET=x86_64-w64-mingw32.static
fi
if [ "$bit" = "-x86" ]; then
#Windows x86 Build
MXE_TARGET=i686-w64-mingw32.static
fi

#No windows timer on default
if [ "$dbg" = "" ]; then
DFLAGS="${DFLAGS} -DDEBUG:STRING=\"-DNTIMER\""
fi
#activate openmp
if [ "$omp" = "-openmp" ]; then
DFLAGS="${DFLAGS} -DUSE_OPENMP=ON"
fi
#prep cmake
${MXE_DIR}/usr/bin/${MXE_TARGET}-cmake . -B${BDIR} -DVERSION:STRING="$(git describe --tags --abbrev=0 --dirty)${bit}${omp}${dbg}"  ${DFLAGS}

#cmake sets dndebug by default => remove that
if [ "$dbg" = "-debug" ]; then
sed -i -e 's/-DNDEBUG//g' ${BDIR}/CMakeCache.txt
fi

# compile
cmake --build ${BDIR}

mv ${BDIR}/tuo.exe ${NAME}

done
done
done

set +xue
