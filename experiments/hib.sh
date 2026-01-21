#! /bin/bash

# ./experiments/run.sh

turns=$(seq 1 30)
workdir=$PWD
resultsdir="$workdir/experiments/results/"
scenarios='30nodes 60nodes 90nodes 120nodes 150nodes 180nodes'

for scenario in $scenarios; 
do
    echo "puHib" > $resultsdir/pused-$scenario-hib.txt
    echo "ppHib" > $resultsdir/ppreempted-$scenario-hib.txt
    echo "msHib" > $resultsdir/makespan-$scenario-hib.txt
    ./experiments/change-input.sh $scenario
    totalofnodes=${scenario%nodes}

    for turn in $turns; 
    do

        ./ns3 run "main --balanced=false --seed=$turn" > log-hib.tmp 2>&1
        cp scratch/database.db $resultsdir/sqlite-$scenario-hib-$turn.db
        sqlite3 scratch/database.db < experiments/pefficient.sql
        mv temp.csv $resultsdir/pefficient-$scenario-hib-$turn.csv
        testUsedHib=$(sqlite3 scratch/database.db < experiments/pused.sql)
        testPreemptedHib=$(sqlite3 scratch/database.db < experiments/ppreempted.sql)
        testMakespanHib=$(sqlite3 scratch/database.db < experiments/makespan.sql)

        echo "$testUsedHib" >> $resultsdir/pused-$scenario-hib.txt
        echo "$testPreemptedHib" >> $resultsdir/ppreempted-$scenario-hib.txt
        echo "$testMakespanHib" >> $resultsdir/makespan-$scenario-hib.txt

    done
    rm log-hib.tmp
done