####################################################################################
###
### janusminer
###
### Hive integration: dmp
###
####################################################################################

#!/usr/bin/env bash
conf=""
conf+="-h $CUSTOM_URL -a $CUSTOM_TEMPLATE"

[[ ! -z $CUSTOM_USER_CONFIG ]] && conf+=" $CUSTOM_USER_CONFIG"

echo "$conf"
echo "$conf" > $CUSTOM_CONFIG_FILENAME

