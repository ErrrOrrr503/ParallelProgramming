#!/bin/bash
if [[ $# -eq 0 ]] ; then
    echo "Usage: ./deploy.sh <source folder> [max run time] "
    exit 255
fi

if [[ $1 = "upload" ]] ; then
    echo "uploading via scp..."
    scp -r -P 52960 $2 b0190302@remote.vdi.mipt.ru:~/
    exit 0
fi

BINFILE=$(basename $1)
for SOURCE_FILE in $(ls ./$1 | grep ".c")
do
    SOURCE_LIST="$SOURCE_LIST $SOURCE_FILE"
done
# generate Makefile that can generate job.sh
rm ./$1/Makefile
printf "\
ifdef NUM_PROC \n\
\tNP=\"-np \$(NUM_PROC)\" \n\
endif \n\
ifdef NODES \n\
\tNDS=\", nodes=\$(NODES)\" \n\
endif \n\
ifdef PPN \n\
\tPN=\", ppn=\$(PPN)\" \n\
endif \n\
ifdef MAX_RUN_TIME \n\
\tWALLTIME=\"walltime=\$(MAX_RUN_TIME)\" \n\
else \n\
\tWALLTIME=\"walltime=00:01:00\" \n\
endif \n\
all: \n\
\tmpicc -o $BINFILE $SOURCE_LIST \n\
\techo -e \"\
\\\\043PBS -l \$(WALLTIME)\$(NDS)\$(PN)\\\\n\
\\\\043PBS -N $BINFILE \\\\n\
\\\\043PBS -q batch \\\\n\
cd \\\\044PBS_O_WORKDIR \\\\n\
mpirun --hostfile \\\\044PBS_NODEFILE \$(NP) ./$BINFILE\"\
\t> job.sh \n\
run: \n\
\tqsub ./job.sh \n\
clean: \n\
\trm $BINFILE ./job.sh \n\
help: \n\
\t@echo -e \"Run make to compile and generate job.sh.\\\\nYou can set\\\\nMAX_RUNTIME=hh:mm:ss\\\\nPPN (process per node)\\\\nNUM_PROC (total processes)\\\\nNODES (number of nodes to use)\\\\nThen use 'make run' to run.\""\
>> ./$1/Makefile
exit 0