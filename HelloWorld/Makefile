ifdef NUM_PROC 
	NP="-np $(NUM_PROC)" 
endif 
ifdef NODES 
	NDS=", nodes=$(NODES)" 
endif 
ifdef PPN 
	PN=", ppn=$(PPN)" 
endif 
ifdef MAX_RUN_TIME 
	WALLTIME="walltime=$(MAX_RUN_TIME)" 
else 
	WALLTIME="walltime=00:01:00" 
endif 
all: 
	mpicc -std=c11 -o HelloWorld  main.c 
	echo -e "\043PBS -l $(WALLTIME)$(NDS)$(PN)\n\043PBS -N HelloWorld \n\043PBS -q batch \ncd \044PBS_O_WORKDIR \nmpirun --hostfile \044PBS_NODEFILE $(NP) ./HelloWorld $(ARGS)"	> job.sh 
run: 
	qsub ./job.sh 
clean: 
	rm HelloWorld ./job.sh 
help: 
	@echo -e "Run make to compile and generate job.sh.\nYou can set\nMAX_RUN_TIME=hh:mm:ss\nPPN (process per node)\nNUM_PROC (total processes)\nNODES (number of nodes to use)\nAnd ARGS for the programm to be run\nThen use 'make run' to run."