#PBS -l walltime=00:01:00 
#PBS -N 2_Sum_1_to_N 
#PBS -q batch 
cd $PBS_O_WORKDIR 
mpirun --hostfile $PBS_NODEFILE  ./2_Sum_1_to_N 
