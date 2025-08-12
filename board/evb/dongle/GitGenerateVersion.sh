#!/bin/bash

# template file
TEMPLATE_FILE="git_version.template"
# generated version file
VERSION_FILE="version.h"

# $1 is the prefix of Git Tag, and default is "bb3-v"
if [ ! $1 ]; then
    TAG_HEADER="bb3-v"
else
    TAG_HEADER=$1
fi

# get version from tag
TAG=`git tag -l "$TAG_HEADER*" | tail -1`
# set version to 0.0.0 if no tag
if [ ! $TAG ]; then
    TAG_VERSION="0.0.0"
else
    TAG_VERSION=`echo ${TAG#$TAG_HEADER}`
fi
#echo $TAG_VERSION

# version = tag + SHA-1(1-9)
PROJ_VERSION="$TAG_VERSION.$(git rev-list HEAD -n 1 | cut -c 1-9)"
#echo $PROJ_VERSION

# split version to four fields
MAJOR=`echo $PROJ_VERSION | awk -F '.' '{print $1}'`
MINOR=`echo $PROJ_VERSION | awk -F '.' '{print $2}'`
REVISION=`echo $PROJ_VERSION | awk -F '.' '{print $3}'`
BUILDNUM=`echo $PROJ_VERSION | awk -F '.' '{print $4}'`

# build time
BUILDING_TIME=`env LANG=en_US.UTF-8 date '+%a %b %e %R:%S %Y'`
#echo $BUILDING_TIME

# substitute version&time in template
`cat $TEMPLATE_FILE | sed -e "s/MAJOR_T/$MAJOR/g" \
                     -e "s/MINOR_T/$MINOR/g" \
                     -e "s/REVISION_T/$REVISION/g" \
                     -e "s/BUILDNUM_T/$BUILDNUM/g" \
                     -e "s/BUILDTIME_T/$BUILDING_TIME/g"    \
                    > $VERSION_FILE`

# show result
echo "Generated Version File :"
cat $VERSION_FILE
