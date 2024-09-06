#!/bin/bash

declare -A TUO_INITIAL_DECKS
declare commander deck
declare -a your_forts
declare -i iters
declare -i min_deck_size max_deck_size
declare sim eiters effect fund endgame

# defaults
iters=500
sim=""
random=""
eiters=""
fund=""
endgame=""
threads=8
min_deck_size=3
max_deck_size=10


### BEGIN OF INITIAL DECKS ###

declare -A TUO_INITIAL_DECKS

TUO_EXP_SETTINGS="$HOME/.tuo-exp${TUO_LOGIN:+.$TUO_LOGIN}"

if ! source "$TUO_EXP_SETTINGS"; then
    echo "No such file (TUO-EXP SETTINGS): $TUO_EXP_SETTINGS" 2>&1
    exit 255
fi

## if 'deck' commander is not set: setup it as empty deck
##  (-D/TUO_DECK should be used for its purposes)
if [[ -z ${TUO_INITIAL_DECKS[deck]+x} ]]; then
    TUO_INITIAL_DECKS[deck]=""
fi

## if 'any' commander is not set: set any first found as 'any'
if [[ -z ${TUO_INITIAL_DECKS[any]+x} ]]; then
    for x in "${TUO_INITIAL_DECKS[@]}"; do
        TUO_INITIAL_DECKS[any]=$x
        break
    done
fi

### END OF INITIAL DECKS ###


show_usage_and_exit() {
    echo "Usage: $0 [options] "Raid Full Name-lvl" [raid_name_alias]
Available options:

    -c COMMANDER (supported: ${!TUO_INITIAL_DECKS[@]})
    -Y YOUR_FORTS
    -e EFFECT
    -D define your deck
    -s use 'sim' mode (default is 'climb'/'climbex')
    -r use 'random' mode (default is 'ordered')
    -i ITERATIONS
    -I EXT_ITERATIONS (enables climbex mode)
    -t THREADS
    -L MIN_DECK_SIZE (default: $min_deck_size)
    -U MAX_DECK_SIZE (default: $max_deck_size)
    -f FLAGS (flag options of tuo)
    -F FUND
    -G End-GAME
" >&2
    exit 255
}

while getopts ":hc:Y:e:D:sri:I:t:L:U:f:F:G:" opt; do
    case $opt in
        h) show_usage_and_exit;;

        c)
            commander=$OPTARG
            if [[ -z $deck ]]; then
                deck=${TUO_INITIAL_DECKS[$commander]}
                if [[ -z $deck ]]; then
                    echo "commander not supported (no initial deck defined): $commander"
                    exit 10
                fi
            fi
        ;;

        Y)  your_forts+=("$OPTARG");;
        e)  effect="$OPTARG";;
        D)  deck=$OPTARG;;
        s)  sim=1;;
        r)  random=1;;
        i)  iters=$OPTARG;;
        I)  eiters=$OPTARG;;
        t)  threads=$OPTARG;;
        L)  min_deck_size=$OPTARG;;
        U)  max_deck_size=$OPTARG;;
        f)  flags+=("$OPTARG");;
        F)  fund=$OPTARG;;
        G)  endgame=$OPTARG;;

        \?)
            echo "Invalid option: -$OPTARG" >&2
            exit 1
        ;;

        :)
            echo "Option -$OPTARG requires an argument." >&2
            exit 1
        ;;
    esac
done

shift $(($OPTIND - 1))
declare enemy_target=$1
declare target=${2:-$enemy_target}

if [[ -z $enemy_target ]]; then
    echo "ERROR: No enemy deck (raid name)" >&2
    exit 1
fi

declare your_forts_plain=${your_forts[@]/%/,}
your_forts_plain=${your_forts_plain%,}

declare -a command
declare log

if [[ -z $effect ]]; then
    echo "WARN: No effect"
fi

if (( sim )); then
    todo="sim"
    eiters=""
else
    todo="climb${eiters:+ex}"
fi

if (( random )); then
    order="random"
else
    order="ordered"
fi

log="tuo-exp-raid.${commander}-vs-${target}.effect[${effect}].${todo}.${order}.log"
if [[ -f $log ]]; then
    echo "log already exists: $log" >&2
    exit 2
fi

options=(
    ${effect:+-e "$effect"}
    ${threads:+-t "$threads"}
    ${fund:+fund "$fund"}
    ${endgame:+endgame "$endgame"}
    ${your_forts_plain:+yfort "${your_forts_plain}"}
    -L $min_deck_size $max_deck_size
    "${flags[@]}" raid "$order" "$todo" "$iters" ${eiters:+"$eiters"}
)

[[ $commander =~ ^any(_|$) ]] || options+=("keep-commander")

command=(tuo.sh "$deck" "$enemy_target" "${options[@]}")

( echo "run: ${command[@]}"; echo ) | tee "$log" >&2
"${command[@]}" &>> "$log" &
