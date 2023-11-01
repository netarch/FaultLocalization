logdir=black_hole_logs

touch ${logdir}/links_to_remove
topofile=../../ns3/topology/ft_k10_os3/ns3ft_deg10_sw125_svr250_os3_i1.edgelist

nfails=1

# for the simulator
outfile_sim=${logdir}/plog_${nfails}
# for the localizer
outfile_loc=${logdir}/temp

echo ${nfails}" "${topofile}" "${outfile_sim}" "${outfile_loc}

time python3 ../../flow_simulator/flow_simulator.py \
    --network_file ${topofile} \
    --nfailures ${nfails} \
    --flows_file ${logdir}/flows \
    --failed_flows_only \
    --outfile ${outfile_sim} > ${logdir}/flowsim_out
echo "flow sim done"

make -j8
max_links=5

if false; then
time ./black_hole_estimator ${outfile_sim} ${topofile} 0.0 10000.01 16  > ${outfile_loc}
cat ${outfile_loc} | grep "Best link to remove" | sed 's/(\|,\|)//g' | awk '{print $5" "$6}' | head -n${max_links} > ${logdir}/links_to_remove
fi

fail_file=${outfile_sim}.fails
inputs=`echo "${fail_file} ${topofile} ${outfile_sim}"`
while read p q; do
    echo "$p $q"
    topofile_mod=${logdir}/ns3ft_deg10_sw125_svr250_os3_i1_r${p}_${q}.edgelist
    echo ${topofile_mod}
    cat ${topofile} | grep -v "${p} ${q}" | grep -v "${q} ${p}"  > ${topofile_mod}
    if false; then
    time python3 ../../flow_simulator/flow_simulator.py \
        --network_file ${topofile_mod} \
        --nfailures ${nfails} \
        --flows_file ${logdir}/flows_r${p}_r${q} \
        --outfile ${outfile_sim}_r${p}_${q} > ${logdir}/flowsim_out_r${p}_${q} \
        --fails_from_file \
        --failed_flows_only \
        --fail_file ${fail_file}
    fi
    inputs=`echo "${inputs} ${topofile_mod} ${outfile_sim}_r${p}_${q}"`
done < ${logdir}/links_to_remove

> ${logdir}/input
echo ${inputs}
echo ${inputs} >> ${logdir}/input
time ./black_hole_estimator_agg 0.0 1000000.01 16 ${inputs} > ${logdir}/temp_agg
echo "2) black_hole_estimator done"

cat ${logdir}/temp_agg | grep "Best link to remove" | sed 's/(\|,\|)//g' | awk '{print $5" "$6}' | head -n${max_links} > ${logdir}/links_to_remove2
while read p q; do
    echo "$p $q"
    topofile_mod=${logdir}/ns3ft_deg10_sw125_svr250_os3_i1_rn${p}_${q}.edgelist
    echo ${topofile_mod}
    cat ${topofile} | grep -v "${p} ${q}" | grep -v "${q} ${p}"  > ${topofile_mod}
    time python3 ../../flow_simulator/flow_simulator.py \
        --network_file ${topofile_mod} \
        --nfailures ${nfails} \
        --flows_file ${logdir}/flows_rn${p}_${q} \
        --outfile ${outfile_sim}_rn${p}_${q} > ${logdir}/flowsim_out_rn${p}_${q} \
        --fails_from_file \
        --failed_flows_only \
        --fail_file ${fail_file}
    inputs=`echo "${inputs} ${topofile_mod} ${outfile_sim}_rn${p}_${q}"`
done < ${logdir}/links_to_remove2

echo "Starting 3) black_hole_estimator done"
echo ${inputs} >> ${logdir}/input

echo ${inputs}
time ./black_hole_estimator_agg 0.0 1000000.01 16 ${inputs} > ${logdir}/temp_agg2
