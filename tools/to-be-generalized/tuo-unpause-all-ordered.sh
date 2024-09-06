#!/bin/bash

ps -efww | fgrep ordered | fgrep './tuo' | sed -r 's:\s+:\t:g' | cut -f2 | xargs kill -CONT
