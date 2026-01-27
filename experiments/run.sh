#! /bin/bash

# ./experiments/run.sh

turns=$(seq 1 30)
workdir=$PWD
resultsdir="$workdir/experiments/results/"
scenarios='30nodes'

for scenario in $scenarios; 
do
    ./experiments/change-input.sh $scenario
    totalofnodes=${scenario%nodes}

    for turn in $turns; 
    do

        ./ns3 run "main --method=bal --seed=$turn" > log-bal.tmp 2>&1
        cp scratch/database.db $resultsdir/sqlite-$scenario-bal-$turn.db

        ./ns3 run "main --method=sat --seed=$turn" > log-sat.tmp 2>&1
        cp scratch/database.db $resultsdir/sqlite-$scenario-sat-$turn.db

        ./ns3 run "main --method=hib --seed=$turn" > log-hib.tmp 2>&1
        cp scratch/database.db $resultsdir/sqlite-$scenario-hib-$turn.db

    done
    rm log-bal.tmp log-sat.tmp
done