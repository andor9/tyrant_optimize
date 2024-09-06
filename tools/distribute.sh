#!/bin/bash

declare -i count=${1:-8}

(( count < 1 )) && exit 1

go() {
    local -i idx=${1:-1}
    (( idx > count )) && cat > /dev/null && return
    exec tee >( awk "{ if ((NR % $count) == $((idx - 1))) { print \$0; } }" > out.${idx}.txt ) | go $((idx + 1))
}

go
