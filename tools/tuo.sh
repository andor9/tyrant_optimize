#!/bin/bash

## prevent running TUO by root
if ((! $(id -u))); then
    echo "!!!! DO NOT RUN TUO.SH BY ROOT !!!!" 1>&2
    exit 255
fi

## init vars / setup defaults
declare -a ARGS=("$@")
declare LOGIN_MAPPING_CONF="${HOME}/.tuo.login-mapping"

[[ -z $TUO_ROOT ]] && TUO_ROOT="$(realpath --logical "$(dirname "$(realpath "$0")")/..")"
[[ -z $TUO_LOGIN ]] && TUO_LOGIN=$(id -un)

## check & export TUO_ROOT
if [[ ! -d $TUO_ROOT ]]; then
    echo "No such TUO directory: TUO_ROOT=$TUO_ROOT" 1>&2
    exit 254
fi
export TUO_ROOT

## load login mapping
[[ -f $LOGIN_MAPPING_CONF ]] && . "$LOGIN_MAPPING_CONF"

## export & remap host logins to tuo logins (nicknames)
export TUO_LOGIN="${LOGIN_MAPPING[$TUO_LOGIN]:-$TUO_LOGIN}"

## 1. setup personal ownedcards_<nickname>.txt (iff isn't set yet)
## 2. detect debug
declare -i found_ownedcards=0
declare found_debug=""
for arg in "${ARGS[@]}"; do
    [[ $arg =~ ^-o[=-] ]] && found_ownedcards=1
    [[ $arg = debug ]] && found_debug="debug"
done
if (( ! found_ownedcards )) && (( TUO_RESTRICT_OWNEDCARDS )); then
    ARGS+=("-o=${TUO_ROOT}/data/ownedcards_${TUO_LOGIN}.txt")
fi


## helper function: quote shell params
quote () {
    local quoted=${1//\'/\'\\\'\'};
    printf "'%s'" "$quoted"
}

## mk exec info
info="EXECUTING: (PWD: ${TUO_ROOT}) ./tuo${found_debug}"
for arg in "${ARGS[@]}"; do
    if ! [[ $arg =~ ^[0-9a-zA-Z_:.,=/-]+$ ]]; then
        arg=$(quote "$arg")
    fi
    info+=" $arg"
done
echo "pid: $$"
echo "timestamp: $(date +%s) ($(date +'%F %T'))"
echo "$info"

## niceness settings (let's make execution less aggressive)
declare -i niceness=15
[[ -n $TUO_DECK ]] && niceness=10

## run tuo (use exec: replace the current process: tuo must have the same pid
## which was logged for managing simulation by using script tuo-kill-by-log.sh)
cd "$TUO_ROOT" && exec nice -n $niceness ./tuo${found_debug} "${ARGS[@]}"
