#!/bin/bash

#INITIAL_DECK="Silus the Corrupt, Constantine's Lab #2, Ridge Terra, Sidewinder Naja, Mangle Loader #3, Atomic Wardriver"
INITIAL_DECK="Daedalus, Abyss Digger, Necrox the Demise, Punctutrops, Loathe Gravewing"

CAMPAIGN_NAME="phobos_heist"

declare -a options=(
    _campaign_${CAMPAIGN_NAME,,}
    surge ordered keep-commander
    climbex 100 5000
    endgame 1
    allow-candidates "Loathe Gravewing"
    #fund 2500
    -t 2
)

LEVEL=3

for i in {1..7}; do
    no="${LEVEL}0${i}"
    name="${CAMPAIGN_NAME}_${no}"
    xopts=("${options[@]}")
    ((i < 7)) && xopts+=(campaign)
    tuo "$INITIAL_DECK" "$name" "${xopts[@]}" &> tuo-campaign.no-vip."${no}".log &
done
