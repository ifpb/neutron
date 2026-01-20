numberofnodes=$1

options='30nodes 60nodes 90nodes 120nodes 150nodes 180nodes'

if [[ " $options " =~ .*\ $numberofnodes\ .* ]]; then
    cd scratch/
    rm input.yaml
    ln -s ./$numberofnodes/input.yaml input.yaml  
else
    echo "invalid option"
fi
