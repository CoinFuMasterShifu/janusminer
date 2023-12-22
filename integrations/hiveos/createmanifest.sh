####################################################################################
###
### janusminer
###
### Hive integration: dmp
###
####################################################################################

if [ "$#" -ne "2" ]
  then
    echo "No arguments supplied. Call using createmanifest.sh <CUSTOM_NAME> <VERSION_NUMBER>"
    exit
fi
cat > h-manifest.conf << EOF
####################################################################################
###
### janusminer
###
### Hive integration: dmp
###
####################################################################################

# The name of the miner
CUSTOM_NAME=$1

# Optional version of your custom miner package
CUSTOM_VERSION=$2
CUSTOM_BUILD=0
CUSTOM_MINERBIN=wart-miner

# Full path to miner config file
CUSTOM_CONFIG_FILENAME=/hive/miners/custom/\$CUSTOM_NAME/config.ini

# Full path to log file basename. WITHOUT EXTENSION (don't include .log at the end)
# Used to truncate logs and rotate,
# E.g. /var/log/miner/mysuperminer/somelogname (filename without .log at the end)
CUSTOM_LOG_BASENAME=/var/log/miner/\$CUSTOM_NAME

EOF
