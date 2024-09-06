#!/bin/bash

FUND=0 #1100 #6000 #4000 #6000 #10000

RUN_BRAWL=0
RUN_ARENA=1

RUN_ATTACK=1
RUN_DEFENSE=1

RANDOM_ATTACK=0

ATTACK_ITERS=(13 200)
DEFENSE_ITERS=(33 500)

((RANDOM_ATTACK)) && ATTACK_ITERS=("${DEFENSE_ITERS[@]}")


ATK_COMMANDERS=(any)
#ATK_COMMANDERS+=(any_{sunder,coalition})
#ATK_COMMANDERS+=(any_{rt,imp,coa})
#ATK_COMMANDERS+=(ded)
#ATK_COMMANDERS+=(const)
#ATK_COMMANDERS+=(nexor)
ATK_COMMANDERS+=(silus)
#ATK_COMMANDERS+=(kylen)
#ATK_COMMANDERS+=(typhon)
#ATK_COMMANDERS+=(krellus)



[[ $TUO_LOGIN == lugofira ]] && ATK_COMMANDERS=(ded nexor nexor any)

#DEF_COMMANDERS=(silus)

ENEMY_DECK="arena_all"
#ENEMY_DECK="Arena"
ENEMY_ALIAS="${ENEMY_DECK,,}"

#ENEMY_DECK="GW_GT_DEF"
#ENEMY_ALIAS="gw_gt_def"

#ENEMY_DEF_DECK="BRAWL_GT_MYTH:0.5;BRAWL_GT_HARD:1.25;BRAWL_GT_NORM:1.0"
#ENEMY_DEF_ALIAS="brawl_gt_medium"

#ENEMY_DEF_DECK="BRAWL_GT_NORM:1.0;BRAWL_GT_EASY:1.5"
#ENEMY_DEF_ALIAS="brawl_gt_easy"

#ENEMY_DEF_DECK="BRAWL_GT_HARD:1.0;BRAWL_GT_NORM:1.5"
#ENEMY_DEF_ALIAS="brawl_gt_norm"

#ENEMY_DEF_DECK="BRAWL_GT_MYTH:1.0;BRAWL_GT_HARD:1.5"
#ENEMY_DEF_ALIAS="brawl_gt_hard"

#ENEMY_DEF_DECK="CQ_GT_HARD:0.5;CQ_GT_NORM:1.0"
#ENEMY_DEF_ALIAS="cq_gt_hard"

# ----------------- BEG WR ---------------------

#ENEMY_DEF_DECK="/^BRAWL_GT__WR_(50_54|55_59|60_64|65_69)$/"
#ENEMY_DEF_ALIAS="brawl_gt_wr60_10"

#ENEMY_DEF_DECK="/^BRAWL_GT__WR_(60_64|65_69|70_74|75_79)$/"
#ENEMY_DEF_ALIAS="brawl_gt_wr70_10"

#ENEMY_DEF_DECK="/^BRAWL_GT__WR_(75_79|80_84)$/"
#ENEMY_DEF_ALIAS="brawl_gt_wr80_5"

#ENEMY_DEF_DECK="/^BRAWL_GT__WR_(70_74|75_79|80_84|85_89|90_94)$/"
#ENEMY_DEF_ALIAS="brawl_gt_wr80_10"

#ENEMY_DEF_DECK="BRAWL_GT__WR_30_34:0.3;BRAWL_GT__WR_35_39:0.4;BRAWL_GT__WR_40_44:0.5;BRAWL_GT__WR_45_49:0.6;BRAWL_GT__WR_50_54:0.5;BRAWL_GT__WR_55_59:0.4"
#ENEMY_DEF_ALIAS="brawl_gt_easy"

ENEMY_DEF_DECK="BRAWL_GT__WR_40_44:0.3;BRAWL_GT__WR_45_49:0.4;BRAWL_GT__WR_50_54:0.5;BRAWL_GT__WR_55_59:0.6;BRAWL_GT__WR_60_64:0.5;BRAWL_GT__WR_65_69:0.4;BRAWL_GT__WR_70_74:0.3"
ENEMY_DEF_ALIAS="brawl_gt_medium"

ENEMY_DEF_DECK="BRAWL_GT__WR_60_64:0.3;BRAWL_GT__WR_65_69:0.4;BRAWL_GT__WR_70_74:0.5;BRAWL_GT__WR_75_79:0.6;BRAWL_GT__WR_80_84:0.5;BRAWL_GT__WR_85_89:0.4;BRAWL_GT__WR_90_99:0.3"
ENEMY_DEF_ALIAS="brawl_gt_hard"

# ----------------- END WR ---------------------

#ENEMY_ATK_DECK="BRAWL_GT_ATK_ALL"
#ENEMY_ATK_ALIAS="${ENEMY_ATK_DECK,,}"

ENEMY_ATK_DECK="BRAWL_GT_ATK_MYTH:1.0;BRAWL_GT_ATK_HERO:1.25;BRAWL_GT_ATK_NORM:0.75"
ENEMY_ATK_ALIAS="brawl_atk_strong"

#ENEMY_ATK_DECK="CQ_GT_ATK_ALL"
#ENEMY_ATK_ALIAS="${ENEMY_ATK_DECK,,}"

#BGE="_FLX"
unset BGE

#ENEMY_DECK="BRAWL_GT${BGE}_ALL"
#ENEMY_ALIAS="brawl_gt_all"

#ENEMY_DECK="BRAWL_GT${BGE}_ATK_ALL"
#ENEMY_ALIAS="brawl_gt_atk"

#ENEMY_DECK="BRAWL_GT${BGE}_FANT"
#ENEMY_ALIAS="brawl_gt_fant"

#ENEMY_DECK="BRAWL_GT${BGE}_FANT:2;BRAWL_GT${BGE}_MYTH:5;BRAWL_GT${BGE}_HERO:3"
#ENEMY_ALIAS="brawl_gt_crazy"

#ENEMY_DECK="BRAWL_GT${BGE}_FANT:1;BRAWL_GT${BGE}_MYTH:2;BRAWL_GT${BGE}_HERO:3;BRAWL_GT${BGE}_NORM:2"
#ENEMY_ALIAS="brawl_gt_strong"

#ENEMY_DECK="BRAWL_GT${BGE}_MYTH:1;BRAWL_GT${BGE}_HERO:2;BRAWL_GT${BGE}_NORM:3;BRAWL_GT${BGE}_EASY:2"
#ENEMY_ALIAS="brawl_gt_normal"

#ENEMY_DECK="BRAWL_GT${BGE}_HERO:1;BRAWL_GT${BGE}_NORM:2;BRAWL_GT${BGE}_EASY:3"
#ENEMY_ALIAS="brawl_gt_easy"


COMMON_OPTIONS=(
    ddd_b64
    _${TUO_LOGIN:-dsuchka}
    _${TUO_LOGIN:-dsuchka}_bb
    #_gw_gt.FA_x_SF_SF
    #_gw_gt_atk.SF_SF_x_FA
    #_mj2
    _brawl_gt
    _brawl_gt_atk
    #_cq_gt2
    #_cq_gt_atk
    #_arena
    endgame 2
    -t 4
    climb-opts:iter-mul=6
    #dom-
    -e "Megamorphosis"
)

#
##  Arena options
#

ARENA_COMMON_OPTIONS=(
)

ARENA_ATTACK_OPTIONS=(
    #climb-opts:open-the-deck
)

ARENA_DEFENSE_OPTIONS=(
    enemy:ordered
)


#
##  Brawl options
#

BRAWL_COMMON_OPTIONS=(
)

BRAWL_ATTACK_OPTIONS=(
    #-L 7 10
)

BRAWL_DEFENSE_OPTIONS=(
    enemy:ordered
)


## setup fund option or unset variable

if ((FUND)); then
    COMMON_OPTIONS+=("fund" "$FUND")
else
    unset FUND
fi


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

[[ -z ${DEF_COMMANDERS+x} ]] && [[ ! -z ${ATK_COMMANDERS+x} ]] && DEF_COMMANDERS=("${ATK_COMMANDERS[@]}")

[[ -z ${ATK_COMMANDERS+x} ]] && ATK_COMMANDERS=("${!TUO_INITIAL_DECKS[@]}")
[[ -z ${DEF_COMMANDERS+x} ]] && DEF_COMMANDERS=("${!TUO_INITIAL_DECKS[@]}")

[[ -z ${ENEMY_DEF_DECK+x} ]] && [[ -n $ENEMY_DECK ]] && ENEMY_DEF_DECK=$ENEMY_DECK
[[ -z ${ENEMY_DEF_ALIAS+x} ]] && [[ -n $ENEMY_ALIAS ]] && ENEMY_DEF_ALIAS=$ENEMY_ALIAS

[[ -z ${ENEMY_ATK_DECK+x} ]] && [[ -n $ENEMY_DECK ]] && ENEMY_ATK_DECK=$ENEMY_DECK
[[ -z ${ENEMY_ATK_ALIAS+x} ]] && [[ -n $ENEMY_ALIAS ]] && ENEMY_ATK_ALIAS=$ENEMY_ALIAS

declare -a OPTIONS
declare -a opts

check_commander() {
    local commander=$1
    if [[ -z ${TUO_INITIAL_DECKS[$commander]} ]]; then
        echo "No such commander: $commander (available commanders: ${!TUO_INITIAL_DECKS[@]})"
        return 255
    fi
    return 0
}

run_command() {
    local log=$1
    shift
    local command=("$@")
    if [[ -e "$log" ]]; then
        echo "log already exists: $log"
        return 1
    fi
    ( echo "run: ${command[@]}"; echo ) | tee "$log" >&2
    "${command[@]}" &>> "$log" &
}

## brawl
if ((RUN_BRAWL)); then
    if ((RUN_ATTACK)); then
        ((RANDOM_ATTACK)) && order="random" || order="ordered"
        OPTIONS=(
            "${COMMON_OPTIONS[@]}" "${BRAWL_COMMON_OPTIONS[@]}" "${BRAWL_ATTACK_OPTIONS[@]}"
            "$order" brawl climbex "${ATTACK_ITERS[@]}"
        )
        for commander in "${ATK_COMMANDERS[@]}"; do
            check_commander "$commander" || continue
            log="tuo-brawl.attack-${order}.${commander}-vs-${ENEMY_DEF_ALIAS}${FUND:+.fund${FUND}}.log"
            opts=("${OPTIONS[@]}")
            [[ $commander =~ ^any(_|$) ]] || opts+=("keep-commander")
            run_command "$log" tuo.sh "${TUO_INITIAL_DECKS[$commander]}" "$ENEMY_DEF_DECK" "${opts[@]}"
        done
    fi
    if ((RUN_DEFENSE)); then
        OPTIONS=(
            "${COMMON_OPTIONS[@]}" "${BRAWL_COMMON_OPTIONS[@]}" "${BRAWL_DEFENSE_OPTIONS[@]}"
            random brawl-defense climbex "${DEFENSE_ITERS[@]}"
        )
        for commander in "${DEF_COMMANDERS[@]}"; do
            check_commander "$commander" || continue
            log="tuo-brawl.defense-random.${commander}-vs-${ENEMY_ATK_ALIAS}${FUND:+.fund${FUND}}.log"
            opts=("${OPTIONS[@]}")
            [[ $commander =~ ^any(_|$) ]] || opts+=("keep-commander")
            run_command "$log" tuo.sh "${TUO_INITIAL_DECKS[$commander]}" "$ENEMY_ATK_DECK" "${opts[@]}"
        done
    fi
fi

## arena
if ((RUN_ARENA)); then
    if ((RUN_ATTACK)); then
        ((RANDOM_ATTACK)) && order="random" || order="ordered"
        OPTIONS=(
            "${COMMON_OPTIONS[@]}" "${ARENA_COMMON_OPTIONS[@]}" "${ARENA_ATTACK_OPTIONS[@]}"
            "$order" fight win climbex "${ATTACK_ITERS[@]}"
        )
        for commander in "${ATK_COMMANDERS[@]}"; do
            check_commander "$commander" || continue
            log="tuo-arena.attack-${order}.${commander}-vs-${ENEMY_DEF_ALIAS}${FUND:+.fund${FUND}}.log"
            opts=("${OPTIONS[@]}")
            [[ $commander =~ ^any(_|$) ]] || opts+=("keep-commander")
            run_command "$log" tuo.sh "${TUO_INITIAL_DECKS[$commander]}" "$ENEMY_DEF_DECK" "${opts[@]}"
        done
    fi
    if ((RUN_DEFENSE)); then
        OPTIONS=(
            "${COMMON_OPTIONS[@]}" "${ARENA_COMMON_OPTIONS[@]}" "${ARENA_DEFENSE_OPTIONS[@]}"
            random surge defense climbex "${DEFENSE_ITERS[@]}"
        )
        for commander in "${DEF_COMMANDERS[@]}"; do
            check_commander "$commander" || continue
            log="tuo-arena.defense-random.${commander}-vs-${ENEMY_ATK_ALIAS}${FUND:+.fund${FUND}}.log"
            opts=("${OPTIONS[@]}")
            [[ $commander =~ ^any(_|$) ]] || opts+=("keep-commander")
            run_command "$log" tuo.sh "${TUO_INITIAL_DECKS[$commander]}" "$ENEMY_ATK_DECK" "${opts[@]}"
        done
    fi
fi
