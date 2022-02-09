#!/bin/bash

# bash script to set the version number for "latest" release builds
# usage: ./ci/set-latest-version.sh "asdfasdgfawerce"
# appends the first 7 chars of supplied string to SPATIAL_MODEL_EDITOR_VERSION

set -e -x

VERSION_SUFFIX=$(echo "$1" | head -c 7)
VERSION_FILE=core/common/src/version.cpp.in

echo "Appending ${VERSION_SUFFIX} to SPATIAL_MODEL_EDITOR_VERSION in ${VERSION_FILE}"

cat ${VERSION_FILE}

sed -i.bak "s/@PROJECT_VERSION@/@PROJECT_VERSION@+${VERSION_SUFFIX}/g" ${VERSION_FILE}

cat ${VERSION_FILE}
