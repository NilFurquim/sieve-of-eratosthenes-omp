#!/usr/bin/env bash

run() {
    local N=4294967294
    local I=1
    # while [ $N -le $MAX ]; do
        TIME=$(date +%s%3N)
        (./sieve $N)  #> /dev/null
        if [ $? -gt 0 ]; then exit; fi
        echo $I. $(($(date +%s%3N) - $TIME))ms  looking until $N
        I=$((I+1))
        N=$(($N*10))
    # done
}

compare() {
    export OMP_NUM_THREADS=1
    echo ********Running with OMP_NUM_THREADS = $OMP_NUM_THREADS********
    run

    echo ""

    unset OMP_NUM_THREADS
    echo ********Running with default OMP_NUM_THREADS********
    run
}

MAX="$(echo "10^$1" | bc)"
IN=$1
COMPILER="gcc sieve.c -lm -fopenmp -Wall -Wextra -Werror -std=c99 -o sieve"

echo "############### SLOW32 ###############"
$COMPILER
compare
echo "######################################"

echo "############### SLOWEX ###############"
$COMPILER -DSIEVE_EXTENDED
compare
echo "######################################"

echo "############### FAST32 ###############"
$COMPILER -DSIEVE_FAST
compare
echo "######################################"

echo "############### FASTEX ###############"
$COMPILER -DSIEVE_FAST -DSIEVE_EXTENDED
compare
echo "######################################"

