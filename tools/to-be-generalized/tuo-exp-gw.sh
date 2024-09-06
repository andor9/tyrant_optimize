#!/bin/bash

declare -A TUO_INITIAL_DECKS
declare commander deck
declare -a your_forts enemy_forts
declare -i iters
declare -i min_deck_size max_deck_size
declare sim eiters fund endgame gbge ybge ebge

# defaults
iters=100
defense=""
sim=""
eiters=""
fund=""
random=""
threads=8
min_deck_size=4
max_deck_size=10


### BEGIN OF INITIAL DECKS ###

declare -A TUO_INITIAL_DECKS

TUO_EXP_SETTINGS="$HOME/.tuo-exp${TUO_LOGIN:+.$TUO_LOGIN}"

if ! source "$TUO_EXP_SETTINGS"; then
    echo "No such file (TUO-EXP SETTINGS): $TUO_EXP_SETTINGS" >&2
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
    echo "Usage: $0 [options] \"EnemyDeck(s)\" [enemy_name_alias]
Available options:

    -c COMMANDER (supported: ${!TUO_INITIAL_DECKS[@]})
    -Y YOUR_FORTS
    -E ENEMY_FORTS
    -e GBGE:YBGE:EBGE
    -D define your deck
    -d use defense mode ('gw-defense', default is 'gw')
    -s use sim mode (default is climb/climbex)
    -r use random mode (default is 'ordered' for non-defense mode)
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
            if [[ -z $deck ]] && [[ $commander != deck ]]; then
                deck=${TUO_INITIAL_DECKS[$commander]}
                if [[ -z $deck ]]; then
                    echo "commander not supported (no initial deck defined): $commander"
                    exit 10
                fi
            fi
        ;;

        Y)  your_forts+=("$OPTARG");;
        E)  enemy_forts+=("$OPTARG");;
        e)  IFS=: read gbge ybge ebge <<< "$OPTARG";;
        D)  deck=$OPTARG;;
        d)  defense=1;;
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
declare -a opts

if [[ -z $enemy_target ]]; then
    echo "ERROR: No enemy deck" >&2
    exit 1
fi

declare your_forts_plain=${your_forts[@]/%/,}
your_forts_plain=${your_forts_plain%,}

declare enemy_forts_plain=${enemy_forts[@]/%/,}
enemy_forts_plain=${enemy_forts_plain%,}

declare -a command
declare log game_type order

if (( defense )) || (( random )); then
    order="random"
else
    order="ordered"
fi

# defense mode
if (( defense )); then
    game_type="defense"
    opts=($order gw-defense)
else
    game_type="attack"
    opts=($order gw)
fi

# simulation mode
if (( sim )); then
    todo="sim"
    eiters=""
else
    todo="climb${eiters:+ex}"
fi

# log file
log="tuo-exp-gw.${game_type}.${commander}-vs-${target}.e[${gbge}:${ybge}:${ebge}].${todo}.${order}.log"
if [[ -f $log ]]; then
    echo "log already exists: $log" >&2
    exit 2
fi

[[ $commander =~ ^any(_|$) ]] || opts+=("keep-commander")

command=(
    tuo.sh "$deck" "$enemy_target" \
        ${your_forts_plain:+yfort "${your_forts_plain}"} \
        ${enemy_forts_plain:+efort "${enemy_forts_plain}"} \
        ${gbge:+-e "$gbge"} \
        ${ybge:+ye "$ybge"} \
        ${ebge:+ee "$ebge"} \
        ${threads:+-t "$threads"} \
        ${fund:+fund "$fund"} \
        ${endgame:+endgame "$endgame"} \
        -L $min_deck_size $max_deck_size \
        "${flags[@]}" "${opts[@]}" \
        "$todo" "$iters" ${eiters:+"$eiters"}
)

( echo "run: ${command[@]}"; echo ) | tee "$log" >&2
"${command[@]}" &>> "$log" &
