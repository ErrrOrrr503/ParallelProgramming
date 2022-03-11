# Parallel Programming homework

**deploy.sh** is usable for generating Makefile from project folder and uploading it to the computing cluster.

    deploy.sh <folder>

generates a Makefile in <folder> from sources in <folder>

    deploy.sh upload <folder>

uploads ready project folder via scp, for now only to mine account.

**Makefile**

    make

compiles and generates job.sh for **qsub**

    make run

runs qsub

Different precesses and numa related parameters can be set. See

    make help

# HelloWorld

Prints "Hello from rank <№/rank"of the process> of <communicator size>"  from all the processes.