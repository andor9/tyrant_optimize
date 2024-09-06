#!/bin/bash

sign=${1:-0}
word=${2}

if [[ -z "$word" ]]; then
    echo "No word (mask for egrep)!!" 1>&2
    exit 1
fi

ps -efww | egrep '\.\/[t]uo' | egrep "$word" \
    | sed -r -e 's: +: :g' | tee >(cat 1>&2) \
    | cut -f2 -d ' ' | tee >(xargs echo kill "-$sign" 1>&2) \
    | xargs --no-run-if-empty kill "-$sign"
