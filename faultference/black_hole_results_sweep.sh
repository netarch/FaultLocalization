iters=5
for i in $(seq $iters)
do
    for topo in "topo_ft_deg14_sw245_svr686_os3_i0"
    do
        for sequence_mode in "Intelligent" "Random"
        do
            for inference_mode in "Flock" "Baseline"
            do
                echo "*** Iteration" $i $sequence_mode $inference_mode $topo
                ./black_hole.sh $sequence_mode $inference_mode $topo
            done
        done
    done
done