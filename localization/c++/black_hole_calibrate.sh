logdir=$1

nfails=1
nthreads=8
maxiter=16

topodir=../../ns3/topology/ft_k12_os3
topoprefix=ns3ft_deg12_sw180_svr432_os3_i1
topofile=${topodir}/${topoprefix}.edgelist
echo ${nfails}" "${topofile}

make -j8
mkdir -p ${logdir}
touch ${logdir}/input
inputs=""

iter=1
while [ ${iter} -le ${maxiter} ]
do

    outdir=${logdir}/run${iter}
    tracefile=${outdir}/plog_${nfails}
    mkdir ${outdir}

    time python3 ../../flow_simulator/flow_simulator.py \
        --network_file ${topofile} \
        --nfailures ${nfails} \
        --flows_file ${outdir}/flows \
        --outfile ${tracefile} > ${outdir}/flowsim_out
    echo "flow sim done"

    inputs=`echo "${inputs} ${topofile} ${tracefile}"`
    (( iter++ ))
done

echo ${inputs}
echo ${inputs} >> ${logdir}/input

time ./black_hole_calibrate 0.0 1000000.01 ${nthreads} ${inputs} > ${logdir}/param