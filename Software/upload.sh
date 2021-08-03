#!/bin/sh
#echo "----------->in shload.sh!!" >> outfile.txt
#echo $@ >> outfile.txt
COUNT=0
MAX_TRIES=4
ERR=4
newFile=$(ls -t | head -n1)

while [ $COUNT -lt $MAX_TRIES ]; do 
    echo uploading ${newFile}
    aws --profile prp --endpoint https://s3.nautilus.optiputer.net s3 cp ${newFile} s3://braingeneers/archive/ephysDebugging/${newFile}
    if [ $? -eq 0 ];then
	exit 0
    fi
	COUNT=$((COUNT+1))
done
echo "Upload fails after 4 trials"


mv ${newFile} data/
exit $ERR
