#!/bin/bash
s=INDEX
sd=SECONDS_SLEEP
szp=$(printf "%09d" $s)
#date
echo "$0: Will sleep for $sd seconds"
echo "$0: Will sleep for $sd seconds" > /tmp/$szp.out
sleep $sd
#date
echo "$0: The script ends"
echo "$0: The script ends" >> /tmp/$szp.out
