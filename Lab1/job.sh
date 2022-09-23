#PBS -l walltime=00:01:00 
#PBS -N Lab1 
#PBS -q batch 
cd $PBS_O_WORKDIR 
mpirun --hostfile $PBS_NODEFILE  ./Lab1 
