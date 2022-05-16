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

Second task. Calculates sum from 1 to N. Accepts N as an argument. Also calculates approximate time of each process working, due to async sending sometimes processes exit after main prints out result.

    project_dir> make ARGS="5"
    project_dir> make run

will calculate 1 + 2 + 3 + 4 + 5.

# 3_ring_communication

Third task. Passes data array between processes in ring topology, in reversed order (to reduce one check as rank0 can always send to last process (maybe also rank0) but not always rank1 exist). Each rank prints it's data element. Data array can be specified an ${NP} command line string arguments, no matter how long each will be. Otherwise data is random symbol 1-char strings array.

    project_dir> make NUM_PROC=4 ARGS="qwe ewq asd123 dsafghlkj"
    project_dir> make run

# 3_star_communication

Third task. Same as *ring_communication* but using star communication: 0 -> N-1 -(ack)> 0 -> N-2 -(ack)> 0 ... 

    project_dir> make NUM_PROC=4 ARGS="qwe ewq asd123 dsafghlkj"
    project_dir> make run

# 4_row_sum

Forth task. Calculates **e** or **pi**. Accepts number of series members and **-pi** of **-e** (default) as arguments.

For **e** Teilor's series is used. For **pi** Newton's methof of pi/2 = SUM k!/(2k + 1)!! is used.

Firstly, part-factorials are precalculated. If each process receive Nproc members, it will Nproc, Nproc*(Nproc + 1), Nproc*(Nproc + 1)(Nproc + 2), ... and then share this with all the others. After that from part-factorials reals factorials of rank predecessors are calculated for O(communicator size). Then member part sums cre calculated and passed to main thread.

For -e rough estimate is O(N_proc(pre-fact) + comm_size(pred_fact) + N_proc(part sums) + comm_size (main thread calculate sum)).

Long double enables handling ~1e6 members for **e** and >500 members for **pi**.

Execution time is measured.

    project_dir> make NUM_PROC=13 ARGS="666 -pi"
    project_dir> make NUM_PROC=13 ARGS="666 -e"
    project_dir> make run

# 5_pthread_hw

Prints "Hello from tid <thread id of the thread>"  from all the initiated threads, except main one - that is the main process. **make** still use qsub to manage runtime.

    project_dir> make ARGS=<number of threads to run>
    project_dir> make run
    project_dir> cat stdout.txt

# 6_pthread_sum

Calculates sum from 1 to N. Accepts number of threads and N as an argument. Also calculates approximate time of each process working. g++ is required due to gcc can't include neither time.h not bits/time.h properly.

    project_dir> make ARGS="13 189"
    project_dir> make run

will calculate 1 + 2 + 3 + 4 + 5 + ... + 189 on 13 threads.

# 7_pthread_integrate

Calculates integral of function F (currently F = x^2). start, end and step should be passed as arguments. Each thread adds it's part to total sum using a semaphore for synchronization.

    project_dir> make ARGS="<number of threads> <start> <end> <step>"
    project_dir> make run