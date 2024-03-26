  ####################################################################################
###
### janusminer
### 
####################################################################################

if [ "$#" -ne "1" ]
  then
    echo "No arguments supplied. Call using createmanifest.sh <VERSION_NUMBER>"
    exit
fi

cat > mmp-external.conf << EOF

####################################################################################
###
### janusminer
###
### MMP integration: Dobromir Dobrev < dobreff@gmail.com >
###
####################################################################################

# Name of external miner
EXTERNAL_NAME="janusminer"

# API Protocol used - TBA.
EXTERNAL_PROTOCOL="miner-script"

# Architecture support for miner damominer
EXTERNAL_ARCH="nvidia"

# Version if provided
EXTERNAL_VERSION=$1

# Miner executable binary
EXTERNAL_BIN="wart-miner"

# Miner command options
EXTERNAL_CMD_OPTS=""

# Specify miner config for startup if available
# EXTERNAL_CONFIG=/opt/mmp/miners/external/damominer/damominer.conf

# If you provide extra params place them below this comment
EXTERNAL_REQUIREMENTS=""

EOF
