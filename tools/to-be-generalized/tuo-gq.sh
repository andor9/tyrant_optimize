#!/bin/bash

BGE=""

YOUR_DECK="Silus the Corrupt, Dreamhaunter, Savant Ascendant, Shock Disruptor #3, Soul Annex, Deacon's Operator, Sierra Upheaval, Syndicate Goliath, Ridge Rider"
YOUR_FORTS=""

ENEMY_DECK="Worldship, Mist Skulker, Morpheus, Spawnsire, Nightmare, Disdainful Impaler, Scuttle Slayer, Incubus, Engulfing Raksha, Flourish Turbine, Hex Descent"
ENEMY_FORTS="Minefield, Forcefield"

tuo.sh "$YOUR_DECK" "$ENEMY_DECK" \
    ${YOUR_FORTS:+yfort "${YOUR_FORTS}"} \
    ${ENEMY_FORTS:+ efort "${ENEMY_FORTS}"} \
    ${BGE:+-e "$BGE"} \
    climbex 100 1000 ordered endgame 0 -t 5
