#!/bin/bash

##
## Run this script to generate lots of scripts and run for a while.
## Use it's output as it's CSV descriptor to run them with bgrunner
##

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
template=$DIR/helper_script.template

echo "#alias;startAfterMS;maxDurationMS;command"

ss=7
sw=4000
for i in $(seq 1 1000); do
  iz=$(printf "%09d" $i)
  if [ $ss -ge 8 ];    then ss=0; fi         # 1..7 seconds sleeping
  if [ $sw -ge 6000 ]; then sw=0; fi         # 1..5 seconds wait before run
  param=$(date "+%Y%m%d_%H%M%S_%N")
  echo "$iz;$sw;6500;/tmp/script.$iz.sh $ss $param"   # 1/8 will timeout
  cat "$template" | sed -e "s/INDEX/$i/" > /tmp/script.$iz.sh
  let ss=ss+1
  let sw=sw+1000
done
