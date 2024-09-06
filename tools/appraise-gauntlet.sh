#!/bin/bash

#SUFFIX="virulence"
#SUFFIX="avenge"
#SUFFIX="flux"
#SUFFIX="TC_x_SF_SF"
#SUFFIX="nexus"
INPUT_FILE="$1"
OUTPUT_FILE="${2:-$(dirname "$1")/appraised${SUFFIX:+.${SUFFIX}}.$(basename "$1")}"

function reverse_forts() { [[ $1 =~ ^(..)_x_(.._..)$ ]] && echo "${BASH_REMATCH[2]}_x_${BASH_REMATCH[1]}"; }

declare -i ITERS=500

#ENEMY="GW_GT_ATK_ALL"
ENEMY="BRAWL_GT_ATK_ALL"
#ENEMY="CQ_GT_ATK_ALL"
#ENEMY="CQ_GT_ATK_HARD:0.5;CQ_GT_ATK_NORM:1.0"
#ENEMY="GW_GT_ATK_HARD:0.75;GW_GT_ATK_NORM:1.0"

TUO_OPTIONS=(
    _brawl_gt_atk
    brawl-defense

    #_brawl_gt
    #brawl
    #ordered

    enemy:ordered
    #-e "Counterflux"

    -t 2

    ## CQ settings
    #fight defense
    #_cq_gt_atk
    #yf "Phobos Station-2, Andar Quarantine-2, The Spire-2, Ashrock Redoubt-2, Baron's Claw Labs-2, SkyCom Complex-2"
    #ef "Phobos Station-2, Andar Quarantine-2, The Spire-2, Ashrock Redoubt-2, Baron's Claw Labs-2, SkyCom Complex-2"

    ## GW settings
    #_gw_gt_atk.$(reverse_forts "$SUFFIX")
    #yf "TC"
    #ef "SF(2)"
    #gw-defense

    # enemy is defender (in surge mode) / with IA(2) against MF / use valid custom BGEs!
    #ee "Tartarian Gift"
    #ye "Progenitor Tech"
)

TMP_DIR="/tmp"
TMP_FILE_PREFIX="."
TMP_FILE_SUFFIX=".appraising.~"

die() {
    echo " ** ERROR ** $@" 1>&2
    exit 255
}

msg() {
    echo "[$(date +%F' '%T)] $@"
}

# check input file
[[ -f $INPUT_FILE ]] || die "Bad input file (no such file or file isn't regular): $INPUT_FILE"

# reset output file or die
:> "$OUTPUT_FILE" || die "Can't open file for writing: $OUTPUT_FILE"

# show start message
msg "Starting appraising for gauntlet $INPUT_FILE (output to $OUTPUT_FILE)"

IFS=$'\n'
while read -r line; do
    if [[ $line =~ (^[a-zA-Z0-9_.+-]+):\ ([^/].+[^/])$ ]]; then
        user="${BASH_REMATCH[1]}"
        deck="${BASH_REMATCH[2]}"
        msg "Appraising $user ..."
        TMP_FILE="${TMP_DIR}/${TMP_FILE_PREFIX}${user}${SUFFIX:+.${SUFFIX}}${TMP_FILE_SUFFIX}"
        [[ -f $TMP_FILE ]] && die "Temporary file already exists: $TMP_FILE"
        tuo.sh "$deck" "$ENEMY" "${TUO_OPTIONS[@]}" sim $ITERS &> "$TMP_FILE" &
        declare -i tuo_pid=$!
        wait $tuo_pid
        res_winrate=$(tail -n10 "$TMP_FILE" | grep -P -o '(?<=win%: )[0-9.]+(?=\s)')
        res_score=$(tail -n10 "$TMP_FILE" | grep -P -o '(?<=score: )[0-9.]+(?=\s)')
        [[ -z $res_winrate ]] && [[ -z $res_score ]] && die "No winrate/score found neither: see $TMP_FILE"
        echo "$(printf "%-7s %%  %-11s" "$res_winrate" "$res_score")$user: $deck" >> "$OUTPUT_FILE"
        rm -rf "$TMP_FILE"
    else
        echo "$line" >> "$OUTPUT_FILE"
    fi
done < "$INPUT_FILE"

# show finish message
msg "Appraising finished"
