logdir=black_hole_logs

> ${logdir}/overall

failfile=`cat $1 | tail -n1 | awk '{print $1}'`
inputfile=$1.temp
echo ${inputfile}

cat ${1} | tail -n1 | awk '{for (i=2; i<NF; i++) printf $i " "; print $NF}' |  tr ' ' '\n' | paste - - > ${inputfile}

inputs=${failfile}
while read p q; do
    inputs=`echo "${inputs} ${p} ${q}"`
    echo "Inputs ${inputs}"
    time ./black_hole_estimator_agg 0.0 1000000.01 16 ${inputs} >> ${logdir}/overall
done < ${inputfile}

