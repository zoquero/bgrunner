#!/bin/bash

##
## Run this script to generate lots of scripts and run for a while.
## Use it's output as it's descriptor to run them with bgrunner
##

template=./helper_script.template.sh

sd=1
for i in $(seq 1 1000); do
  iz=$(printf "%09d" $i)
  cat "$template" | sed -e "s/INDEX/$i/"  | sed -e "s/SECONDS_SLEEP/$sd/" > /tmp/script.$iz.sh
  let sd=sd+1

  echo "$iz;1234;9999;/tmp/script.$iz.sh"
# echo "$iz;9999;/tmp/script.$iz.sh"

  if [ $sd -eq 8 ]; then sd=1; fi
done
