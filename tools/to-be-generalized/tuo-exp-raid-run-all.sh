#!/bin/bash

raid_name="Council of Sages Raid"
effect="Megamorphosis"
#levels=({14..14})
#levels=({09..14})
#levels=({09..25})
#levels=({16..18})
#levels=({15..20})
#levels=({14..25})
#levels=({15..25})
levels=({23..25})

#levels=({20..30})
#levels=({45..49})

declare -i FUND=550 #1100 #5000 #10000
declare -i ITERS_FROM=2000
declare -i ITERS_TILL=100000
declare -i THREADS=4
declare -i ENDGAME=2

declare -A yforts=(
    #[ia_ia]="Inspiring Altar #2"
    #[lc_lc]="Lightning Cannon #2"
    #[sf_sf]="Sky Fortress #2"
    #[cs_cs]="Corrosive Spore #2"
    #[df_df]="Death Factory #2"
    #[ds_ds]="Darkspire #2"
    #[mc_mc]="Medical Center #2"

    #[xx_xx]=""

    #[cs3_lc3]="Corrosive Spore-3, Lightning Cannon-3"
    #[lc3_lc3]="Lightning Cannon-3(2)"
    [lc1_lc1]="Lightning Cannon-1(2)"
    #[lc_lc3]="Lightning Cannon, Lightning Cannon-3"
    #[ia1_ia1]="Inspiring Altar-1(2)"
    #[cs_lc]="Corrosive Spore, Lightning Cannon"
    #[cs_df]="Corrosive Spore, Death Factory"
    #[cs_ia]="Corrosive Spore, Inspiring Altar"
    #[cs_mt]="Corrosive Spore, Mortar Tower"
    #[lc_df]="Lightning Cannon, Death Factory"
    #[lc_ia]="Lightning Cannon, Inspiring Altar"
)

commanders=(
    silus
    #typhon
    any
    #any_rt
    #any_imp
    #any_coa

    #silus
    #const
    #nexor
    #kylen
    #krellus
    #ded
    #any
    #any_rt
    #any_bt
    #any_imp
)
if [[ $TUO_LOGIN == "lugofira" ]]; then
    commanders=(
        ded
        nexor+
        nexor
        any
    )
fi


for level in "${levels[@]}"; do
    for yfort in "${!yforts[@]}"; do
        for commander in "${commanders[@]}"; do
            tuo-exp-raid.sh \
                -c "$commander" \
                -Y "${yforts[$yfort]}" \
                -e "$effect" \
                -i $ITERS_FROM -I $ITERS_TILL \
                -t $THREADS \
                -G $ENDGAME -F $FUND \
                -f "_${TUO_LOGIN:-dsuchka}" \
                "${raid_name}-${level##0}" "raid_${level}_${yfort}"
        done
    done
done
