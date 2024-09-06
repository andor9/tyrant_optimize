#!/bin/bash

declare -i FUND=2200 #4000
declare -i ENDGAME=2

# phase 1
gbge="Megamorphosis"

# phase 2
#gbge=""

# phase 3
#gbge=""


ybges=(
    ""
    #"Winter Tempest"
    #"Orbital Cannon"
    #"Charged Up"
    #"Combined Arms"
    #"Ferocity"
    #"Tartarian Gift"
    #"Artillery"
    #"Blightblast"
    #"Divine Blessing"
    #"Inspired"
    #"Progenitor Tech"
    #"Triage"
    #"Emergency Aid"
    #"Mirror Madness"
    #"Sulfuris Essence"
)

ebges=(
    ""
    #"Winter Tempest"
    #"Orbital Cannon"
    #"Charged Up"
    #"Combined Arms"
    #"Ferocity"
    #"Tartarian Gift"
    #"Artillery"
    #"Blightblast"
    #"Divine Blessing"
    #"Inspired"
    #"Progenitor Tech"
    #"Triage"
    #"Emergency Aid"
    #"Mirror Madness"
)


defense=""
if [[ $1 == "defense" ]]; then
    defense=1
    shift
fi

declare -A atk_yforts=(
    #[ia_ia]="Inspiring Altar #2"
    #[lc_lc]="Lightning Cannon #2"
    [sf_sf]="Sky Fortress #2"
    #[lc_lc3]="Lightning Cannon, Lightning Cannon-3"

    #[cs_cs]="Corrosive Spore #2"
    #[cs_lc]="Corrosive Spore, Lightning Cannon"
    #[cs_df]="Corrosive Spore, Death Factory"
    #[cs_sf]="Corrosive Spore, Sky Fortress"
    #[cs_ia]="Corrosive Spore, Inspiring Altar"
    #[df_df]="Death Factory #2"
    #[lc_df]="Lightning Cannon, Death Factory"

    #[cs_mt]="Corrosive Spore, Mortar Tower"

    #[sf_sf3]="Sky Fortress, Sky Fortress-3"
    #[sf_lc3]="Sky Fortress, Lightning Cannon-3"
    #[df3_df3]="Death Factory-3(2)"
    #[cs_cs]="Corrosive Spore #2"
    #[cs_df]="Corrosive Spore, Death Factory"
    #[cs_ia]="Corrosive Spore, Inspiring Altar"

    #[xx_xx]=""
)

declare -A def_yforts=(
    #[mf_xx]="Minefield"
    #[tc_xx]="Tesla Coil"
    #[fa_xx]="Foreboding Archway"
    #[ff_xx]="Forcefield"
    #[ib_xx]="Illuminary Blockade"

    #[tc3_xx]="Tesla Coil-3"
    #[ff3_xx]="Forcefield-3"
    #[ib3_xx]="Illuminary Blockade-3"

    #[tc_tc]="Tesla Coil #2"
    #[mf_mf]="Minefield #2"
    #[fa_fa]="Foreboding Archway #2"
    [ff_ff]="Forcefield #2"
    #[ib_ib]="Illuminary Blockade #2"
    #[ff_ib]="Forcefield, Illuminary Blockade"

    #[xx_xx]=""
)

declare -A atk_eforts=(
    #[lc_lc]="Lightning Cannon #2"
    [sf_sf]="Sky Fortress #2"
    #[cs_cs]="Corrosive Spore #2"
    #[cs_sf]="Corrosive Spore, Sky Fortress"
    #[ia_ia]="Inspiring Altar #2"
    #[df_df]="Death Factory #2"
    #[mc_mc]="Medical Center #2"

    #[ia_xx]="Inspiring Altar"
    #[lc_xx]="Lightning Cannon"


    #[cs_ia]="Corrosive Spore, Inspiring Altar"
    #[cs_lc]="Corrosive Spore, Lightning Cannon"
    #[ia_lc]="Inspiring Altar, Lightning Cannon"
    #[df_lc]="Death Factory, Lightning Cannon"
    #[cs_df]="Corrosive Spore, Death Factory"
    #[df3_df]="Death Factory-3, Death Factory"
    #[df_lc]="Death Factory, Lightning Cannon"
    #[sf3_sf3]="Sky Fortress-3(2)"
    #[cs_mt]="Corrosive Spore, Mortar Tower"

    #[xx_xx]=""
)

declare -A def_eforts=(
    #[ib_xx]="Illuminary Blockade"
    [ff_xx]="Forcefield"
    #[mf_xx]="Minefield"
    #[fa_xx]="Foreboding Archway"
    #[tc_xx]="Tesla Coil"

    #[fa_ff]="Foreboding Archway, Forcefield"

    #[mf_mf]="Minefield #2"
    #[ff3_xx]="Forcefield-3"
    #[tc3_xx]="Tesla Coil-3"
    #[fa3_xx]="Foreboding Archway-3"

    #[tc_tc]="Tesla Coil #2"
    #[fa_fa]="Foreboding Archway #2"
    #[ff_ff]="Forcefield #2"
    #[ib_ib]="Illuminary Blockade #2"
    #[ib_ib3]="Illuminary Blockade, Illuminary Blockade-3"
    #[ib_ff3]="Illuminary Blockade, Forcefield-3"

    ## attack forts
    #[ia_xx]="Inspiring Altar"
    #[lc_xx]="Lightning Cannon"
    #[df_xx]="Death Factory"

    #[xx_xx]=""
)

declare -A yforts eforts
declare -a XOPTIONS

#ENEMY_DECK="BRAWL_GT_MYTH:0.7;BRAWL_GT_HARD:0.9;BRAWL_GT_NORM:1.1"
#ENEMY_ALIAS="brawl_gt_hard"

#ENEMY_DECK="BRAWL_GT_HARD:0.7;BRAWL_GT_NORM:1.1;BRAWL_GT_EASY:0.8"
#ENEMY_ALIAS="brawl_gt_norm"

#ENEMY_DECK="BRAWL_GT_ATK_MYTH:0.75;BRAWL_GT_ATK_HERO:1.0;BRAWL_GT_ATK_NORM:0.85"
#ENEMY_ALIAS="brawl_gt_atk_strong"

ENEMY_DECK="GW_GT_HARD;GW_GT_NORM"
ENEMY_ALIAS="gw_gt_strong"

#ENEMY_DECK="GW_GT_HARD:0.5;GW_GT_NORM:1.0;GW_GT_EASY:0.75"
#ENEMY_ALIAS="gw_gt_medium"

#ENEMY_DECK="GW_GT_ATK_ALL"
#ENEMY_ALIAS="gw_gt_atk"

#ENEMY_DECK="GW_GT_ATK_HARD;GW_GT_ATK_NORM"
#ENEMY_ALIAS="gw_gt_atk_strong"

#BGE="_DEVOUR"
#unset BGE

#ENEMY_DECK="BRAWL_GT${BGE}_FANT:1;BRAWL_GT${BGE}_MYTH:2;BRAWL_GT${BGE}_HERO:5;BRAWL_GT${BGE}_NORM:3"
#ENEMY_ALIAS="brawl_gt_strong"

#ENEMY_DECK="BRAWL_GT${BGE}_MYTH:1;BRAWL_GT${BGE}_HERO:3;BRAWL_GT${BGE}_NORM:2"
#ENEMY_ALIAS="brawl_gt_medium"

COMMON_XOPTIONS=(
    -f _${TUO_LOGIN:-dsuchka}
    -f _${TUO_LOGIN:-dsuchka}_bb
    #-f _brawl_gt
    #-f _brawl_gt_atk
    #-f _arena
    -f _gw_gt_atk.SF_SF_x_FA
    -f _gw_gt.FA_x_SF_SF
    -t 2
    #-f dom-
)

commanders=(
    any
    any_imp
    any_counter
    typhon
    silus
    krellus

    #nexor
    #ded
    #kylen
    #const
    #any
)

if [[ $TUO_LOGIN == "lugofira" ]]; then
    commanders=(
        ded
        #nexor
        #typhon
        #any
    )
fi


if (( $defense )); then
    for x in "${!def_yforts[@]}"; do
        yforts[$x]="${def_yforts[$x]}"
    done
    for x in "${!atk_eforts[@]}"; do
        eforts[$x]="${atk_eforts[$x]}"
    done

    XOPTIONS=(
        "${COMMON_XOPTIONS[@]}"
        -i 25${TUO_DECK:+00} -I 250
        -d
        -f enemy:ordered
    )
else
    for x in "${!atk_yforts[@]}"; do
        yforts[$x]="${atk_yforts[$x]}"
    done
    for x in "${!def_eforts[@]}"; do
        eforts[$x]="${def_eforts[$x]}"
    done

    XOPTIONS=(
        "${COMMON_XOPTIONS[@]}"
        -i 10${TUO_DECK:+00} -I 100
    )
fi


if [[ -n $TUO_DECK ]]; then
    commanders=(deck)
fi

for yfort in "${!yforts[@]}"; do
    for efort in "${!eforts[@]}"; do
        for ybge in "${ybges[@]}"; do
            for ebge in "${ebges[@]}"; do
                for commander in "${commanders[@]}"; do
                    tuo-exp-gw.sh \
                        -c "$commander" \
                        -Y "${yforts[$yfort]}" \
                        -E "${eforts[$efort]}" \
                        -e "${gbge}:${ybge}:${ebge}" \
                        -G "$ENDGAME" -F "$FUND" \
                        -L 3 -U 10 \
                        ${TUO_DECK:+-s -D "$TUO_DECK"} \
                        "${XOPTIONS[@]}" \
                        "${ENEMY_DECK}" "${ENEMY_ALIAS}.${yfort}.${efort}"
                done
            done
        done
    done
done
