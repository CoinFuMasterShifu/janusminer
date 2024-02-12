####################################################################################
###
### janusminer
###
### Hive integration: dmp
###
####################################################################################

#!/usr/bin/env bash

if [ -z "$CUSTOM_URL" ]; then
    HOST_URL="localhost"
    HOST_PORT="3000"
elif [[ ! $CUSTOM_URL =~ :[0-9]+$ ]]; then
    HOST_URL="$CUSTOM_URL"
    HOST_PORT="3000"
elif [[ $CUSTOM_URL =~ :([0-9]+)$ ]]; then
    HOST_URL="${CUSTOM_URL%:*}"
    HOST_PORT="${BASH_REMATCH[1]}"
elif [[ $CUSTOM_URL =~ ^([^:]+)://([^/]+):([0-9]+)$ ]]; then
    HOST_URL="${BASH_REMATCH[1]}://${BASH_REMATCH[2]}"
    HOST_PORT="${BASH_REMATCH[3]}"
else
    echo "Invalid input."
fi
conf=""

if [[ $CUSTOM_TEMPLATE == "null" ]]; then
    echo "Wallet address is not specified"
    conf+="-h $HOST_URL -p $HOST_PORT"
else
    if [[ $CUSTOM_TEMPLATE == *.* ]]; then
        USER="${CUSTOM_TEMPLATE##*.}"
        WALLET="${CUSTOM_TEMPLATE%.*}"
        conf+="-h $HOST_URL -p $HOST_PORT -u $CUSTOM_TEMPLATE"
    else
        WALLET="${CUSTOM_TEMPLATE%.*}"
        conf+="-h $HOST_URL -p $HOST_PORT -a $WALLET"
    fi
fi




[[ ! -z $CUSTOM_USER_CONFIG ]] && conf+=" $CUSTOM_USER_CONFIG"

echo "$conf"
echo "$conf" > $CUSTOM_CONFIG_FILENAME

