#!/bin/bash

set -x

echo "$@"

id=$(echo $1 | cut -f3 -d'/')
cookie=$(mktemp)

curl -c $cookie -qgb "" -fLC - --retry 3 --retry-delay 3 "https://drive.google.com/uc?export=download&id=$id" > /dev/null
curl -qgb $cookie -fLC - --retry 3 --retry-delay 3 "https://drive.google.com/uc?export=download&confirm=`awk '/download/ {print $NF}' $cookie`&id=$id" -o $2
