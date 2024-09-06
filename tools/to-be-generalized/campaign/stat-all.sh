#!/bin/bash

tail -n1 *.no-vip.*log \
    | fgrep 'Optimized Deck' \
    | sed -r -e 's/^Optimized Deck: .*[0-9.]+: [a-zA-Z0-9 ]+, //g' -e 's:, :\n:g' \
    | sort | uniq -c | sort -nr
