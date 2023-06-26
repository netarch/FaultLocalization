logdir=$1

topodir=../../ns3/topology/ft_k12_os3
topoprefix=ns3ft_deg12_sw180_svr432_os3_i1

#topoprefix=ns3ft_deg10_sw125_svr250_os3_i1
topofile=${topodir}/${topoprefix}.edgelist

nfails=1
nthreads=8
maxiter=10

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

eq_devices=""

while [ ${iter} -le ${maxiter} ]
do
    echo ${inputs}
    echo ${inputs} >> ${logdir}/input
    time ./black_hole_estimator_agg 0.0 1000000.01 ${nthreads} ${inputs} > ${logdir}/temp_agg_${iter}
    # cat ${logdir}/temp_agg_${iter} | grep "Best link to remove" | sed 's/(\|,\|)//g' | awk '{print $5" "$6}' | head -n${max_links} > ${logdir}/links_to_remove_${iter}
    cat ${logdir}/temp_agg_${iter} | grep "Best link to remove" | sed 's/(//'g | sed 's/)//'g | sed 's/,//'g | awk '{print $5" "$6}' | head -n${max_links} > ${logdir}/links_to_remove_${iter}
    new_eq_devices=`cat ${logdir}/temp_agg_${iter} | grep "equivalent devices" | grep "equivalent devices" | sed 's/equivalent devices //' | sed 's/size.*//'`
    if [[ "${new_eq_devices}" == "${eq_devices}" ]]
    then
        eq_size=`echo ${eq_devices} | sed 's/]//'g | sed 's/\[//'g | awk -F',' '{print NF}'`
        if [[ ${eq_size} -le 1 ]]
        then 
            break
        fi
    fi
    eq_devices=${new_eq_devices}
    while read p q; do
        echo "$p $q"
        suffix=iter${iter}_r${p}_${q}
        topofile_mod=${logdir}/${topoprefix}_${suffix}.edgelist
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
