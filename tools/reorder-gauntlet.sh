#!/bin/bash

#SUFFIX="virulence"
#SUFFIX="avenge"
#SUFFIX="flux"
#SUFFIX="SF_SF_x_TC"
INPUT_FILE="$1"
OUTPUT_FILE="${2:-$(dirname "$1")/reordered${SUFFIX:+.${SUFFIX}}.$(basename "$1")}"

declare -i ITERS=25

#ENEMY="GW_GT_ALL"
#ENEMY="CQ_GT_ALL"
ENEMY="BRAWL_GT_ALL"
#ENEMY="arena_all"

TUO_OPTIONS=(
    brawl
    _brawl_gt

    -t 2

    #-e "Counterflux"

    ## CQ settings
    #surge win
    #_cq_gt
    #yf "Phobos Station-2, Andar Quarantine-2, The Spire-2, Ashrock Redoubt-2, Baron's Claw Labs-2, SkyCom Complex-2"
    #ef "Phobos Station-2, Andar Quarantine-2, The Spire-2, Ashrock Redoubt-2, Baron's Claw Labs-2, SkyCom Complex-2"

    ## GW settings
    #_gw_gt_def
    #yf "SF(2)"
    #ef "TC"
    #gw

    # enemy is defender (in surge mode) / with IA(2) against MF / use valid custom BGEs!
    #ee "Divine Blessing"
    #ye "Divine Blessing"
)

TMP_DIR="/tmp"
TMP_FILE_PREFIX="."
TMP_FILE_SUFFIX=".$(basename "$1" ".txt").reordering~"

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
msg "Starting reorder for gauntlet $INPUT_FILE (output to $OUTPUT_FILE)"

IFS=$'\n'
while read -r line; do
    if [[ $line =~ (^[a-zA-Z0-9_.+-]+[^:]+):\ ([^/].+[^/])$ ]]; then
        user="${BASH_REMATCH[1]}"
        deck="${BASH_REMATCH[2]}"
        msg "Reordering $user ..."
        TMP_FILE="${TMP_DIR}/${TMP_FILE_PREFIX}${user}${SUFFIX:+.${SUFFIX}}${TMP_FILE_SUFFIX}"
        [[ -f $TMP_FILE ]] && die "Temporary file already exists: $TMP_FILE"
        tuo.sh "$deck" "$ENEMY" "${TUO_OPTIONS[@]}" reorder $ITERS &> "$TMP_FILE" &
        declare -i tuo_pid=$!
        wait $tuo_pid
        last_line=$(tail -n1 "$TMP_FILE")
        if [[ $last_line =~ ^Optimized\ Deck:.*\ ([0-9.]+)(\ \[([0-9.]+)\ per\ win\])?:\ ([^:]+)$ ]]; then
            opt_score="${BASH_REMATCH[1]}"
            opt_win_score="${BASH_REMATCH[3]}"
            opt_deck="${BASH_REMATCH[4]}"
            echo "$(printf "%-11s" "$opt_score")$user: $opt_deck" >> "$OUTPUT_FILE"
            rm -rf "$TMP_FILE"
        else
            die "Failed at $user (deck: $deck), see $TMP_FILE for more details"
        fi
    else
        echo "$line" >> "$OUTPUT_FILE"
    fi
done < "$INPUT_FILE"

# show finish message
msg "Reordering finished"
