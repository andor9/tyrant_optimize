#!/bin/bash

for ydef in {mf,fa,tc,ff,ib}_xx; do
    for eatk in {lc_lc,df_df,ia_ia}; do
        printf "\n\n *** YOUR DEF: [ $ydef ]  /  ENEMY ATK: [ $eatk ]\n\n";
        tuo-log-show-stat-gw.py *defense*${ydef}*${eatk}*.e\[*:*:*\].*log;
    done;
done
