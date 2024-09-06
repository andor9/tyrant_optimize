#!/bin/bash
for fn in fusion_recipes_cj2 missions levels skills_set `seq -f cards_section_%g 1 19` ; do
  curl https://mobile$1.tyrantonline.com/assets/${fn}.xml -R -z data/${fn}.xml -o data/${fn}.xml
done
