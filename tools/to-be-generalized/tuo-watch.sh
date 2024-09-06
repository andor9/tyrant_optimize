#!/bin/bash

mask=${1:-tuo*log}

exec watch "tail -n2 $mask | sed -r -e 's:\(([0-9.]+-[0-9.]+, )?([0-9]+ )+\/ ([0-9]+)\):(\1... / \3):'"
