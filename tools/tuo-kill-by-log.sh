#!/bin/bash

sign=${1:-0}
shift

check_tuo_pid() {
    local -i pid=$1
    local cmdline="/proc/$pid/cmdline"
    [[ -r "$cmdline" ]] || return 1
    cmd=$(basename $(tr '\0' '\n' < "$cmdline" | head -n1))
    [[ $cmd == tuo ]] || return 2
    return 0
}

for i in "$@"; do
    xname=$(basename "$i")
    pid=$(head -n5 "$i" | grep -a -P -o '(?<=^pid: )\d+')
    #echo "raw pid: <$pid>"
    if ! check_tuo_pid "$pid"; then
        echo "$xname => $pid (dead)" 1>&2
        continue
    fi
    swap=$(cat /proc/$pid/status | fgrep VmSwap | sed -r -e 's/^VmSwap:\s*//g')
    echo "$xname => $pid (kill -${sign}) (swap: $swap)" 1>&2
    kill -"${sign}" "$pid"
    case "${sign^^}" in
        9|15|TERM|KILL)
            ##echo "removing log: $i" 1>&2
            ##rm -rf "$i"
        ;;
    esac
done
