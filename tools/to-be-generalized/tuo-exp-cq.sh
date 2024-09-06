#!/bin/bash

declare -A TUO_INITIAL_DECKS
declare commander deck
declare -a your_forts enemy_forts effects
declare -i iters
declare -i min_deck_size max_deck_size
declare defense mode eiters fund endgame

# defaults
iters=100
defense=""
mode="climb"
eiters=""
fund=""
endgame=""
threads=8
min_deck_size=3
max_deck_size=10

declare -i tzoffset=$(( ( $(date +%:::z) ) * 3600 ))
declare -i dstoffset=$(( 1 * 3600 ))
declare -i match_period=$(( 4 * 3600 ))
declare -i aligned_4h_time=$(( ( ($(date +%s) + tzoffset - dstoffset) / match_period ) * match_period - tzoffset + dstoffset ))


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
    echo "Usage: $0 [options] enemy_target [enemy_target_alias]
Available options:

    -c COMMANDER (supported: ${!TUO_INITIAL_DECKS[@]}; 'any' disables 'keep-commander' mode; 'deck' is stub for -D option)
    -Y YOUR_FORTS
    -E ENEMY_FORTS
    -e EFFECT (per experiment, allowed empty string as no effect, multiple effects can be separated by comma)
    -D define your deck manually
    -d use defense mode (random+fight+defense; default is attack: ordered+surge+win)
    -s use sim mode (default is climb{ex}, it disables reorder mode)
    -r use reorder mode (default is climb{ex}, it disables sim mode)
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

[[ -z $1 ]] && show_usage_and_exit

while getopts ":hc:Y:E:e:D:dsri:I:t:L:U:f:F:G:" opt; do
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
        E)  enemy_forts+=("$OPTARG");;
        e)  effects+=("$OPTARG");;
        D)  deck=$OPTARG;;
        d)  defense=1;;
        s)  mode="sim";;
        r)  mode="reoder";;
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
declare -a game_mode_opts
declare todo=$mode

declare your_forts_plain=${your_forts[@]/%/,}
your_forts_plain=${your_forts_plain%,}

declare enemy_forts_plain=${enemy_forts[@]/%/,}
enemy_forts_plain=${enemy_forts_plain%,}

declare -a command xeffects
declare log game_type

if (( ! ${#effects[@]} )); then
    echo "WARN: No effects"
    effects+=("")
fi

for effect in "${effects[@]}"; do
    if (( defense )); then
        game_type="defense"
        game_mode_opts=(random fight defense)
    else
        game_type="attack"
        game_mode_opts=(ordered surge win)
    fi

    case $mode in
        sim)
            eiters=""
        ;;

        climb)
            todo="climb${eiters:+ex}"
        ;;

        reorder)
            :
        ;;

        *)
            echo "unknown mode: $mode"
            exit 255
        ;;
    esac

    log="tuo-exp-cq.${game_type}.${commander}-vs-${target}.effect[${effect}].${todo}.at-$(date '+%F-%H' -d @${aligned_4h_time}).log"
    if [[ -f $log ]]; then
        echo "log already exists: $log" >&2
        exit 2
    fi

    OLD_IFS=$IFS
    IFS=$'\n'
    xeffects=()
    for xeffect in $(echo "$effect" | sed -r -e 's:\s*,\s*:\n:g'); do
        xeffects+=(-e "$xeffect")
    done
    IFS=$OLD_IFS

    tuo_options=(
        ${your_forts_plain:+yfort "${your_forts_plain}"}
        ${enemy_forts_plain:+efort "${enemy_forts_plain}"}
        "${xeffects[@]}"
        ${threads:+-t "$threads"}
        ${fund:+fund "$fund"}
        ${endgame:+endgame "$endgame"}
    )

    [[ $commander != any ]] && tuo_options+=("keep-commander")

    tuo_options+=(
        "${flags[@]}" "${game_mode_opts[@]}"
        -L $min_deck_size $max_deck_size
        "$todo" "$iters" ${eiters:+"$eiters"}
    )

    command=(
        tuo.sh "$deck" "$enemy_target" "${tuo_options[@]}"
    )

    #echo "log: $log"
    #log="/dev/stdout"
    ( echo "run: ${command[@]}"; echo ) | tee "$log" >&2
    "${command[@]}" &>> "$log" &
done
