#!/bin/bash

INPUT_FILE="$1"

die() {
    echo " ** ERROR ** $@" 1>&2
    exit 255
}

# check input file
[[ -f $INPUT_FILE ]] || die "Bad input file (no such file or file isn't regular): $INPUT_FILE"

prev_user=""
declare -i number=0
IFS=$'\n'
while read -r line; do
    if [[ $line =~ ^([0-9.]+\ +)?([a-zA-Z0-9_.+-]+_)[0-9]+:\ ([^/].+[^/])$ ]]; then
        score="${BASH_REMATCH[1]}"
        user="${BASH_REMATCH[2]}"
        deck="${BASH_REMATCH[3]}"
        [[ $user != $prev_user ]] && number=0
        let number++
        echo "${score}${user}$(printf "%03d" $number): $deck"
        prev_user=$user
    else
        echo "$line"
    fi
done < "$INPUT_FILE"
