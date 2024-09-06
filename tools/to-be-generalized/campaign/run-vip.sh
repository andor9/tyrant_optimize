#!/bin/bash

commander="Silus the Corrupt"

curr_dir=$(realpath "$(dirname "$0")")
ownedcards_path="${curr_dir}/ownedcards.txt"

CAMPAIGN_NAME="phobos_heist"
LEVEL=2
STAGE_FROM=7
STAGE_TO=7

function join_array() {
    echo -n "$1"
    shift
    while test -n "$1"; do
        echo -n ", $1"
        shift
    done
}

declare -a INITIAL_CARDS=(
    "$commander"
    "Wyrm of the Pit"
    "Wyrm of the Pit"
    "Yurich's Observatory"
    "Sidewinder Naja"
    "Sidewinder Naja"
    "Orbo the Wrathful"
)

declare -a VIP_CARDS_X03=(
    "Erebus Keeper"         # 94->81
    "Magnum Turbo"          # 94->90
    "Loathe Gravewing"      # 95->85
)

declare -a VIP_CARDS_X06=(
    "Orbo the Wrathful"     # 95->88
    "Wyrm of the Pit"       # 95->88 (x2)
)

declare -a VIP_CARDS_X07=(
)

for i in $(seq ${STAGE_FROM} ${STAGE_TO:-${STAGE_FROM}}); do
    no="${LEVEL}0${i}"
    l="${CAMPAIGN_NAME}_${no}"
    log="tuo-campaign.vip${no}.log"

    declare -a VIP_CARDS=()
    ((i < 2)) && ((${#VIP_CARDS_X02[@]} > 0)) && VIP_CARDS+=("${VIP_CARDS_X02[@]}")
    ((i < 3)) && ((${#VIP_CARDS_X03[@]} > 0)) && VIP_CARDS+=("${VIP_CARDS_X03[@]}")
    ((i < 4)) && ((${#VIP_CARDS_X04[@]} > 0)) && VIP_CARDS+=("${VIP_CARDS_X04[@]}")
    ((i < 5)) && ((${#VIP_CARDS_X05[@]} > 0)) && VIP_CARDS+=("${VIP_CARDS_X05[@]}")
    ((i < 6)) && ((${#VIP_CARDS_X06[@]} > 0)) && VIP_CARDS+=("${VIP_CARDS_X06[@]}")
    ((i < 7)) && ((${#VIP_CARDS_X07[@]} > 0)) && VIP_CARDS+=("${VIP_CARDS_X07[@]}")

    INITIAL_DECK="$(join_array "${INITIAL_CARDS[@]}")"
    VIP_CARDS_LIST="$(join_array "${VIP_CARDS[@]}")"

    declare -a xopts=(
        ${VIP_CARDS_LIST:+vip "${VIP_CARDS_LIST}"}
        _campaign_${CAMPAIGN_NAME,,}
        surge ordered
        -t 2
        climbex 100 5000
    )
    ((i < 7)) && xopts+=(campaign)

    declare -a command=(tuo "$INITIAL_DECK" "$l" -o="${ownedcards_path}" "${xopts[@]}")
    echo
    echo "$l: log: $log"
    echo "run command:"
    echo "$(declare -p command)"
    echo
    "${command[@]}" &> "$log" &
done
echo
