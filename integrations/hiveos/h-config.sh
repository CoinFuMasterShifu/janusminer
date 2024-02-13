####################################################################################
###
### janusminer
###
### Hive integration: dmp
###
####################################################################################

#!/usr/bin/env bash

conf=""

if [[ $CUSTOM_URL == "null" ]]; then
    echo "Host is not specified"
elif [[ ! $CUSTOM_URL =~ :[0-9]+$ ]]; then
    HOST_URL="$CUSTOM_URL"
    conf+="-h $HOST_URL"
elif [[ $CUSTOM_URL =~ :([0-9]+)$ ]]; then
    HOST_URL="${CUSTOM_URL%:*}"
    HOST_PORT="${BASH_REMATCH[1]}"
    conf+="-h $HOST_URL -p $HOST_PORT"
elif [[ $CUSTOM_URL =~ ^([^:]+)://([^/]+):([0-9]+)$ ]]; then
    HOST_URL="${BASH_REMATCH[1]}://${BASH_REMATCH[2]}"
    HOST_PORT="${BASH_REMATCH[3]}"
    conf+="-h $HOST_URL -p $HOST_PORT"
else
    echo "Invalid input."
fi


if [[ $CUSTOM_TEMPLATE == "null" ]]; then
    echo "Wallet address is not specified"
else
    if [[ $CUSTOM_TEMPLATE == *.* ]]; then
        USER="${CUSTOM_TEMPLATE##*.}"
        WALLET="${CUSTOM_TEMPLATE%.*}"
        conf+=" -u $CUSTOM_TEMPLATE"
    else
        WALLET="${CUSTOM_TEMPLATE%.*}"
        conf+=" -a $WALLET"
    fi
fi

[[ ! -z $CUSTOM_USER_CONFIG ]] && conf+=" $CUSTOM_USER_CONFIG"

echo "$conf"
echo "$conf" > $CUSTOM_CONFIG_FILENAME

