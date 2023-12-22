####################################################################################
###
### janusminer
###
### Hive integration: dmp
###
####################################################################################

if [ "$#" -ne "2" ]
  then
    echo "No arguments supplied. Call using build.sh <CUSTOM_NAME> <VERSION_NUMBER>"
    exit
fi

cd `dirname $0`
./createmanifest.sh $1 $2
mkdir $1
cp h-* wart-miner $1
tar czvf $1-$2.tgz $1
