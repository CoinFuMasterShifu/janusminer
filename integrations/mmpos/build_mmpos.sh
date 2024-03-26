####################################################################################
###
### janusminer
###
####################################################################################

if [ "$#" -ne "2" ]
  then
    echo "No arguments supplied. Call using build.sh <CUSTOM_NAME> <VERSION_NUMBER>"
    exit
fi

cd `dirname $0`
./createconf.sh $2
mkdir $1
cp mmp-* wart-miner $1
tar czvf $1_$2.tar.gz $1
