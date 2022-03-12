# Parallel Programming homework

**deploy.sh** is usable for generating Makefile from project folder and uploading it to the computing cluster.

    root_repo_dir> deploy.sh <folder>

generates a Makefile in <folder> from sources in <folder>

    root_repo_dir> deploy.sh upload <folder>

uploads ready project folder via scp, for now only to mine account.

**Makefile**

    project_dir> make

compiles and generates job.sh for **qsub**

    project_dir> make run

runs qsub.

Different precesses and numa related parameters can be set. See

    project_dir> make help

Example that launches project with argument argv[1]="1984", using 4 nodes 1 process per node with 4 process total:

    project_dir> make MAX_RUNTIME=00:07:30 PPN=1 NODES=4 NUM_PROC=4 ARGS="1984"

# HelloWorld

Prints "Hello from rank <№/rank"of the process> of <communicator size>"  from all the processes.

    project_dir> make
    project_dir> make run

# 2_Sum_1_to_N

Second task. Calculates sum from 1 to N. Accepts N as an argument

    project_dir> make ARGS="5"
    project_dir> make run

will calculate 1 + 2 + 3 + 4 + 5.