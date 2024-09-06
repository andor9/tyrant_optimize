#!/bin/bash

curr_dir=$(dirname "$0")
olds_dir="$curr_dir/old-xmls.d"

#BASE_URL="https://mobile.tyrantonline.com/assets/"
BASE_URL="https://mobile-dev.tyrantonline.com/assets/"
FILES=(
   fusion_recipes_cj2.xml
   missions.xml
   skills_set.xml
   cards_section_{1..18}.xml
   items.xml
   levels.xml
   codex.xml
   achievements.xml
)

TMP_PREFIX="."
TMP_SUFFIX="~"

ETG_PREFIX="."
ETG_SUFFIX=".etg"

function etag_extract_hash() {
    etag=$1
    etag="${etag##\"}"
    etag="${etag%%\"}"
    echo "${etag##[0-9a-zA-Z]*-}"
}

[[ -d $olds_dir ]] || mkdir -p "$olds_dir"

for f in "${FILES[@]}"; do
    tmp_file="${curr_dir}/${TMP_PREFIX}${f}${TMP_SUFFIX}"
    etg_file="${curr_dir}/${ETG_PREFIX}${f}${ETG_SUFFIX}"
    old_etag=$(cat "${etg_file}" 2>/dev/null)
    new_etag=$(
        curl -X HEAD "${BASE_URL}${f}" --silent &>/dev/null \
            -D >(tr -d '\r' | grep --color=never -P -o '(?<=^ETag: ).*$' > /dev/stdout) \
            -H "Connection: close" --ignore-content-length
    )
    echo
    echo "next file: $f; etag={old: $old_etag, new: $new_etag}" \
            " (Using only hash part for comparing, format: epoch(hex)-hash(hex))" 1>&2
    if [[ -z $new_etag ]]; then
        echo "No found ETag for $f" 1>&2
        exit 1
    fi
    #echo "extracted values: $(etag_extract_hash $old_etag) != $(etag_extract_hash $new_etag)"
    if [[ -z $old_etag ]] \
            || [[ $(etag_extract_hash $old_etag) != $(etag_extract_hash $new_etag) ]]
    then
        etg_tmp_file="${etg_file}${TMP_SUFFIX}"
        out_file="${curr_dir}/${f}"
        old_file="${olds_dir}/${f}"
        echo "$new_etag" > "${etg_tmp_file}"
        wget -O "${tmp_file}" "${BASE_URL}${f}" \
            && (test -f "${out_file}" && mv "${out_file}" "${old_file}" || true) \
            && mv "${tmp_file}" "${out_file}" \
            && mv "${etg_tmp_file}" "${etg_file}" \
        ;
    else
        echo "No ETag($old_etag) changes for $f, skipping" 1>&2
    fi
    echo
done
