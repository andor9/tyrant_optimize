#!/bin/bash

yforts=""
eforts=""
mode=${1:-both}

#yforts="Andar Quarantine-3, The Spire-3, Ashrock Redoubt-3, Baron's Claw Labs-2, SkyCom Complex-2"
#yforts="Phobos Station-3, Ashrock Redoubt-3, Baron's Claw Labs-3, Andar Quarantine-3, Norhaven-2"
yforts="Phobos Station-3, Andar Quarantine-3, SkyCom Complex-3, Spire-3, Ashrock Redoubt-3"
eforts="Phobos Station-3, The Spire-3, Ashrock Redoubt-3, Baron's Claw Labs-2, SkyCom Complex-2, Andar Quarantine-2"

attack_effects=(
    -e ""
    #-e "Phobos Station"

    #-e "Andar Quarantine"
    #-e "The Spire"
    #-e "Ashrock Redoubt"
    #-e "Baron's Claw Labs"
    #-e "SkyCom Complex"

    #-e "Colonial Relay"
    #-e "Mech Graveyard"
    #-e "Infested Depot"
    #-e "Seismic Beacon"
    #-e "Tyrolian Outpost"

    #-e "Red Maw Base"
    #-e "Brood Nest"
    #-e "Borean Forges"
    #-e "Magma Foundry"
)

defense_effects=(
    -e ""
)

flags=(
    -f _${TUO_LOGIN:-dsuchka}
    -f _cq_gt_atk
    -f _cq_gt2
    -f _bb_lv1
    -f "climb-opts:iter-mul=6,use-all-card-levels"
    #-f dom-
    -G 2 #1
    -F 550 #6000 #1000 #6000
)

attack=(
    -t 4
    -i 13${TUO_DECK:+00}
    -I 200
)

defense=(
    -t 4
    -i 33${TUO_DECK:+00}
    -I 500
    -d
    -f enemy:ordered
)

declare -A attack_enemies defense_enemies

# enemy def-decks for 'attack' simming
attack_enemies=(
    #[cq_top]="CQ_GT_MYTH:0.5;CQ_GT_HERO:1.25;CQ_GT_NORM:0.75"
    #[cq_low]="CQ_GT_HERO:0.5;CQ_GT_NORM:1.25;CQ_GT_EASY:0.75"
    [cq_top]="CQ_GT_HARD:0.5;CQ_GT_NORM:0.75"
    [cq_low]="CQ_GT_NORM:0.5;CQ_GT_EASY:0.75"
)

# enemy atk-decks for 'defense' simming
defense_enemies=(
    [cq_atk]="CQ_GT_ATK_HARD:0.5;CQ_GT_ATK_NORM:1.0"
)

case "$TUO_LOGIN" in
    (dsuchka|"")
        attack_commanders=(
            any
            any_rt
            typhon
            #const
            #krellus
            #any_imp
        )
        ;;

    (lugofira)
        attack_commanders=(
            ded
            nexor
        )
        ;;

esac

### BEGIN OF INITIAL DECKS ###

declare -A TUO_INITIAL_DECKS

TUO_EXP_SETTINGS="$HOME/.tuo-exp${TUO_LOGIN:+.$TUO_LOGIN}"

if ! source "$TUO_EXP_SETTINGS"; then
    echo "No such file (TUO-EXP SETTINGS): $TUO_EXP_SETTINGS" 2>&1
    exit 255
fi

## if 'any' commander is not set: set any first found as 'any'
if [[ -z ${TUO_INITIAL_DECKS[any]+x} ]]; then
    for x in "${TUO_INITIAL_DECKS[@]}"; do
        TUO_INITIAL_DECKS[any]=$x
        break
    done
fi

### END OF INITIAL DECKS ###

[[ -z ${defense_commanders+x} ]] && [[ ! -z ${attack_commanders+x} ]] && defense_commanders=("${attack_commanders[@]}")

[[ -z ${attack_commanders+x} ]] && attack_commanders=("${!TUO_INITIAL_DECKS[@]}")
[[ -z ${defense_commanders+x} ]] && defense_commanders=("${!TUO_INITIAL_DECKS[@]}")

if [[ -n $TUO_DECK ]]; then
    attack_commanders=(deck)
    defense_commanders=(deck)
fi


if [[ $mode = attack ]] || [[ $mode = both ]]; then
    for enemy in "${!attack_enemies[@]}"; do
        for commander in "${attack_commanders[@]}"; do
            tuo-exp-cq.sh ${TUO_DECK:+-s -D "$TUO_DECK"} -c $commander \
                -Y "$yforts" -E "$eforts" \
                "${attack_effects[@]}" "${attack[@]}" "${flags[@]}" "${attack_enemies[$enemy]}" "$enemy"
        done
    done
fi
if [[ $mode = defense ]] || [[ $mode = both ]]; then
    for enemy in "${!defense_enemies[@]}"; do
        for commander in "${defense_commanders[@]}"; do
            tuo-exp-cq.sh ${TUO_DECK:+-s -D "$TUO_DECK"} -c $commander \
                -Y "$yforts" -E "$eforts" \
                "${defense_effects[@]}" "${defense[@]}" "${flags[@]}" "${defense_enemies[$enemy]}" "$enemy"
        done
    done
fi
