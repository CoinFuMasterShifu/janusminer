#!/usr/bin/env bash
# This file will contain functions related to gathering stats and displaying it for agent
# Agent will have to call mmp-stats.sh which will contain triggers for configuration files and etc.
# Do not add anything static in this file
GPU_COUNT=$1
LOG_FILE=$2
cd `dirname $0`
. mmp-external.conf


get_bus_ids() {
    local vendor_id="$1"
    local bus_ids=$(/bin/lspci -n | awk '$1 !~ /^00:/ && $2 ~ /^0300|0302:/ && $3 ~ /^'"${vendor_id}"':/ {print $1}')
    local format_bus_ids=()

    if [ -z "$bus_ids" ]; then
        exit 1
    fi

    while read -r bus_id; do
        local format_bus_id=${bus_id%%:*}
        format_bus_ids+=("$format_bus_id")
    done <<< "$bus_ids"

    echo "${format_bus_ids[*]}"
}

get_cpu_hashes() {
    hash=''
    local khs=$(cat "$LOG_FILE" | grep 'Total hashrate (CPU): [0-9.].*mh/s'| sed -n 's/.*: \([0-9.]\+\) mh\/s.*/\1/p'|tail -n1)
    if [[ -z "$khs" ]]; then
        khs="0"
    fi
    if (( $(echo "$khs > 0" | bc -l) )); then
        hash_cpu=$khs
    fi
}

get_cards_hashes() {
    hash=''
    for (( i=0; i < $GPU_COUNT; i++ )); do
        hash[$i]=''
        local mhs=$(cat "$LOG_FILE" | grep "\[${i}\].*mh/s"| sed -n 's/.*: \([0-9.]\+\) mh\/s.*/\1/p'|head -n1)
        if [[ -z "$mhs" ]]; then
            mhs="0"
        fi
        if (( $(echo "$mhs > 0" | bc -l) )); then
            hash_gpu[$i]=$mhs
        fi
    done
}

get_miner_shares_ac(){
    ac=0
    local ac=$(cat "$LOG_FILE" |grep Mined | head -n 1 | awk '{print substr($(NF-2), 1, length($(NF-2))-1)}')
    if [[ -z "$ac" ]]; then
        local ac="0"
    fi
    echo $ac
}

get_miner_shares_rj(){
    rj=0
    local rj=$(cat "$LOG_FILE" |grep Mined | head -n 1 | awk '{print substr($NF, 1, length($NF)-1)}')
    if [[ -z "$rj" ]]; then
        local rj="0"
    fi
    echo $rj
}

get_miner_stats() {
    stats=

    local amd_bus_ids=$(get_bus_ids "1002")
    local nv_bus_ids=$(get_bus_ids "10de")
    local intel_bus_ids=$(get_bus_ids "8086")

# busid generation for both CPU and GPU
busid=( "cpu" )
busid+=( "${amd_bus_ids[@]}" )
busid+=( "${nv_bus_ids[@]}" )
busid+=( "${intel_bus_ids[@]}" )

# Construct the JSON string for busid
json_busid="[\"${busid[0]}\","
for ((i = 1; i < ${#busid[@]}; i++)); do
    IFS=' ' read -ra ids <<< "${busid[$i]}"
    for id in "${ids[@]}"; do
        json_busid+="\"$id\", "
    done
done
json_busid="${json_busid%, }"
json_busid+="]"

    # hashrate gathering
    local hash_cpu
    get_cpu_hashes
    local hash_gpu=()
    get_cards_hashes
    local hash=()
    hash+=( "${hash_cpu[@]}" )
    hash+=( "${hash_gpu[@]}" )
    local units='mhs'                    # hashes units
    # A/R shares by pool
    local ac=$(get_miner_shares_ac)
    local rj=$(get_miner_shares_rj)

    stats=$(jq -nc \
            --argjson hash "$(echo ${hash[*]} | tr " " "\n" | jq -cs '.')" \
	    --argjson busid "$json_busid" \
            --arg units "$units" \
            --arg ac "$ac" --arg inv "0" --arg rj "$rj" \
            --arg miner_version "$CUSTOM_VERSION" \
            --arg miner_name "$CUSTOM_NAME" \
        '{$busid, $hash, $units, air: [$ac, $inv, $rj], miner_name: $miner_name, miner_version: $miner_version}')
    # total hashrate in khs
    echo $stats
}
get_miner_stats $GPU_COUNT $LOG_FILE
