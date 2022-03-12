#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>

typedef unsigned long long tnum;

int main (int argc, char* argv[])
{
    tnum N = (tnum) atoll (argv[1]);
    assert (N >= 0);
    MPI_Init (&argc, &argv);
    int rank = -1, comm_size = -1;
    MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    if (argc != 2 && !rank) {
        printf ("Usage: mpirun %s <N>\n", argv[0]);
        return 1;
    }
    double time_start = MPI_Wtime ();
    //first processes  N/nproc + 1, rest will receive N/nproc.
    tnum N_nproc = N / comm_size;
    tnum N_nproc_1 = N_nproc + 1;
    tnum N_rest = N % comm_size;
    tnum S = 0;
    if (rank < N_rest) {
        tnum start = N_nproc_1 * rank + 1;
        for (tnum num = start; (num - start) < N_nproc_1; S += num, num++) ;
    }
    else {
        tnum start = N_nproc_1 * N_rest + N_nproc * (rank - N_rest) + 1;
        for (tnum num = start; (num - start) < N_nproc; S += num, num++) ;
    }
    MPI_Request unused_request;
    MPI_Isend (&S, sizeof (tnum), MPI_CHAR, 0, 0, MPI_COMM_WORLD, &unused_request); //send async to rank 0
    double time_rank_end = MPI_Wtime ();
    if (rank)
        printf ("Process %d finished in = %.10f seconds\n", rank, time_rank_end - time_start);
    else
        printf ("Main process finished in = %.10f seconds, waiting for others\n", time_rank_end - time_start);
    if (!rank) {
        //managing process
        tnum temp_sum = 0;
        printf ("Using %d processes\n", comm_size);
        for (int i = 1; i < comm_size; i++) {
            MPI_Recv (&temp_sum, sizeof (tnum), MPI_CHAR, i, 0, MPI_COMM_WORLD, NULL);
            S += temp_sum;
        }
        double time_end = MPI_Wtime ();
        printf ("Task finished in = %.10f seconds\n", time_end - time_start);
        printf ("Sum = %llu\n", S);
    }
    MPI_Finalize ();
    return 0;
}