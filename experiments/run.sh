#! /bin/bash

# ./experiments/run.sh

turns=$(seq 1 30)
workdir=$PWD
resultsdir="$workdir/experiments/results/"
scenarios='30nodes 60nodes 90nodes 120nodes 150nodes 180nodes'

for scenario in $scenarios; 
do
    # echo "pfBal,peBal,pfSat,peSat" > $resultsdir/pefficient-$scenario.txt
    echo "puBal,puSat" > $resultsdir/pused-$scenario.txt
    echo "ppBal,ppSat" > $resultsdir/ppreempted-$scenario.txt
    echo "msBal,msSat" > $resultsdir/makespan-$scenario.txt
    ./experiments/change-input.sh $scenario
    totalofnodes=${scenario%nodes}

    for turn in $turns; 
    do

        ./ns3 run "main --balanced=true --seed=$turn" > log-bal.tmp 2>&1
        cp scratch/database.db $resultsdir/sqlite-$scenario-bal-$turn.db
        sqlite3 scratch/database.db < experiments/pefficient.sql
        mv temp.csv $resultsdir/pefficient-$scenario-bal-$turn.csv
        testUsedBal=$(sqlite3 scratch/database.db < experiments/pused.sql)
        testPreemptedBal=$(sqlite3 scratch/database.db < experiments/ppreempted.sql)
        testMakespanBal=$(sqlite3 scratch/database.db < experiments/makespan.sql)

        ./ns3 run "main --balanced=false --seed=$turn" > log-sat.tmp 2>&1
        cp scratch/database.db $resultsdir/sqlite-$scenario-sat-$turn.db
        sqlite3 scratch/database.db < experiments/pefficient.sql
        mv temp.csv $resultsdir/pefficient-$scenario-sat-$turn.csv
        testUsedSat=$(sqlite3 scratch/database.db < experiments/pused.sql)
        testPreemptedSat=$(sqlite3 scratch/database.db < experiments/ppreempted.sql)
        testMakespanSat=$(sqlite3 scratch/database.db < experiments/makespan.sql)

        echo "$testUsedBal,$testUsedSat" >> $resultsdir/pused-$scenario.txt
        echo "$testPreemptedBal,$testPreemptedSat" >> $resultsdir/ppreempted-$scenario.txt
        echo "$testMakespanBal,$testMakespanSat" >> $resultsdir/makespan-$scenario.txt

    done
    rm log-bal.tmp log-sat.tmp
done