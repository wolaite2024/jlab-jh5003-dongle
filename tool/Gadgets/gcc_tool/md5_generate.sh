#!/bin/bash

SOURCE_BIN="$1"
SOURCE_DIR=`dirname $SOURCE_BIN`
SOURCE_BASE_NAME=`basename  $SOURCE_BIN`

SOURCE_NAME=`echo $SOURCE_BASE_NAME|cut -d '.' -f1`
VERSION_FILE="$2"
echo $SOURCE_BIN
MD5_STR=`md5sum.exe $SOURCE_BIN | cut -c 1-32`
#echo $MD5_STR

FILE_CONTENT=
for LINE in `head -n 5 $VERSION_FILE`
do
    FILE_CONTENT=$FILE_CONTENT%$LINE
done
#echo $FILE_CONTENT

MAJOR=`echo $FILE_CONTENT | awk -F '%' '{print $4}'`
MINOR=`echo $FILE_CONTENT | awk -F '%' '{print $7}'`
REVISION=`echo $FILE_CONTENT | awk -F '%' '{print $10}'`
BUILDNUM=`echo $FILE_CONTENT | awk -F '%' '{print $13}'`
GCID=`echo $FILE_CONTENT | awk -F '%' '{print $16}' | cut -c 3-`
#CUSTOMER_NAME=`echo $FILE_CONTENT | awk -F '%' '{print $16}'`
# echo $MAJOR
# echo $MINOR
# echo $REVISION
# echo $BUILDNUM
# echo $GCID
# echo $CUSTOMER_NAME

IMAGE_NAME=$SOURCE_DIR/$SOURCE_NAME-v$MAJOR.$MINOR.$REVISION.$BUILDNUM-$GCID-$MD5_STR.bin
echo $IMAGE_NAME
mv $SOURCE_BIN $IMAGE_NAME
#rm -f "./bin/"$1".bin"
#git checkout $VERSION_FILE
