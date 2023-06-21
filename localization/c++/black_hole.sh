logdir=$1

#topofile=../../ns3/topology/ft_k10_os3/ns3ft_deg10_sw125_svr250_os3_i1.edgelist
topofile=../../ns3/topology/ft_k12_os3/ns3ft_deg12_sw180_svr432_os3_i1.edgelist

nfails=1

# for the simulator
outfile_sim=${logdir}/plog_${nfails}

echo ${nfails}" "${topofile}" "${outfile_sim}

time python3 ../../flow_simulator/flow_simulator.py \
    --network_file ${topofile} \
    --nfailures ${nfails} \
    --flows_file ${logdir}/flows \
    --outfile ${outfile_sim} > ${logdir}/flowsim_out
    # --failed_flows_only \
echo "flow sim done"


make -j8
max_links=1
fail_file=${outfile_sim}.fails

> ${logdir}/input
inputs=`echo "${fail_file} ${topofile} ${outfile_sim}"`
echo "Inputs "${inputs}

iter=1
while [ ${iter} -le 15 ]
do
    echo ${inputs}
    echo ${inputs} >> ${logdir}/input
    time ./black_hole_estimator_agg 0.0 1000000.01 16 ${inputs} > ${logdir}/temp_agg_${iter}
    cat ${logdir}/temp_agg_${iter} | grep "Best link to remove" | sed 's/(\|,\|)//g' | awk '{print $5" "$6}' | head -n${max_links} > ${logdir}/links_to_remove_${iter}
    while read p q; do
        echo "$p $q"
        suffix=iter${iter}_r${p}_${q}
        topofile_mod=${logdir}/ns3ft_deg12_sw180_svr432_os3_i1_${suffix}.edgelist
        echo ${topofile_mod}
        cat ${topofile} | grep -v "${p} ${q}" | grep -v "${q} ${p}"  > ${topofile_mod}
        time python3 ../../flow_simulator/flow_simulator.py \
            --network_file ${topofile_mod} \
            --nfailures ${nfails} \
            --flows_file ${logdir}/flows_${suffix} \
            --outfile ${outfile_sim}_${suffix} > ${logdir}/flowsim_out_${suffix} \
            --fails_from_file \
            --fail_file ${fail_file}
            # --failed_flows_only \
        inputs=`echo "${inputs} ${topofile_mod} ${outfile_sim}_${suffix}"`
    done < ${logdir}/links_to_remove_${iter}
    (( iter++ ))
done
