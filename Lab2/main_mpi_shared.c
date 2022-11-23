#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>
#include <math.h>

const char *mpi_file = "mpi_res.txt";

typedef double time_run_t;

#define MPI_TAG_EDGE 0
#define MPI_TAG_GATHER 0

int fetch_sz = 2;

double* gather_vertical_submatrix (double *a, const int i_size, const int j_size, const int j_for_rank);
time_run_t gather_max_time (time_run_t time);

int main (int argc, char **argv)
{
    MPI_Init (NULL, NULL);
    int rank = -1, comm_size = -1;
    MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    if (argc != 3) {
        if (!rank)
            printf ("usage: mpirun -c <threads> %s <i size> <j_size>", argv[0]);
        return -1;
    }
    int i_size = atoi (argv[1]);
    int j_size = atoi (argv[2]);
    /*
     * The best way would be to divide threads by 2 or 3 groups for
     * emulating nested parallelism in openmp - parallelize both ext and int cycle.
     * group 1 is 0 3 6 ... group 3 is 1 4 7 ... group 3 is 2 5 8 ...
     * responsible for i i + 1 i+2 iterations.
     * That results in 3 times less sends/recvs if compared with internal cycle only parallelisation.
     * BUT in this case there are problems:
     * 3 threads are equal to 4 and 5. and so on
     * calculated data is dramatically sparce. How to gather?
     * We can synch when writing to file, but it will const N^2 send/recvs.
     * Also complex memory distribution. Either sparce parts or parts with divided by 3 indices.
     * Group by 2 can't be used due to i-3th row belongs to other process.
     * Shared memory solves troubles with gathering and memory distribution.
     * Also it enables groups by 2.
     * Also it lessens send/recvs by 2 (now we don't need to pass 2 elements, only indicate they are ready)
     * Parallising only internal cycle makes it easier to distribute memory and task, to gather it (linear fragments are passed)
     * Implementing internal parallelism.
    */
    // out algo will firstly calculate 2 start points and send them, then run through his part
    // then receive and finish row calc.
    // We will NOT consider the case of less than 4 points for each runner.
    int j_for_rank = (j_size - fetch_sz) / comm_size;
    int int_num_rest = (j_size - fetch_sz) % comm_size;
    int int_rank_shift = rank * j_for_rank;
    if (rank >= int_num_rest)
        int_rank_shift = int_num_rest * (j_for_rank + 1) + (rank - int_num_rest) * j_for_rank;
    if (rank < int_num_rest)
        j_for_rank++;
    if (rank == comm_size - 1)
        j_for_rank += fetch_sz;
    double *a = (double *) calloc (i_size * j_for_rank, sizeof (double));
    //подготовительная часть – заполнение некими данными
    for (int i = 0; i < i_size; i++) {
        for (int j = 0; j < j_for_rank; j++) {
            a[i * j_for_rank + j] = 10 * i + j + int_rank_shift;
        }
    }
    // task
    MPI_Request unused_request;
    double a_i_3_recv[2];
    time_run_t time_start = MPI_Wtime ();
    for (int i = 3; i < i_size; i++)
    {
        // for rest threads
        double *a_i = a + i * j_for_rank;
        double *a_i_3 = a + (i - 3) * j_for_rank;
        if (rank)
            MPI_Isend(a_i_3, 2, MPI_DOUBLE, rank - 1, MPI_TAG_EDGE, MPI_COMM_WORLD, &unused_request);
        for (int j = 0; j < j_for_rank - fetch_sz; j++)
            a_i[j] = sin (3 * a_i_3[j + fetch_sz]);
        if (rank < comm_size - 1) {
            MPI_Recv(a_i_3_recv, 2, MPI_DOUBLE, rank + 1, MPI_TAG_EDGE, MPI_COMM_WORLD, NULL);
            for (int j = j_for_rank - fetch_sz; j < j_for_rank; j++)
                a_i[j] = sin (3 * a_i_3_recv[j - j_for_rank + fetch_sz]);
        }
    }
    time_run_t time_calc = MPI_Wtime () - time_start;

    // gather time and figure out MAX time_calc.
    time_run_t max_time = gather_max_time (time_calc);
    if (!rank)
        printf ("%f\n", max_time);

    //gather result
    double *res = gather_vertical_submatrix (a, i_size, j_size, j_for_rank);

    if (!rank) {
        FILE *ff = fopen(mpi_file,"w");
        for(int i = 0; i < i_size; i++){
            for (int j = 0; j < j_size; j++) {
                fprintf(ff,"%f ", res[i * j_size + j]);
            }
            fprintf(ff,"\n");
        }
        fclose(ff);
    }
    free (res);

    MPI_Finalize ();
    return 0;
}

time_run_t gather_max_time (time_run_t time)
{
    int rank = -1, comm_size = -1;
    MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    time_run_t *times = calloc (comm_size, sizeof (time_run_t));
    times[rank] = time;
    for (int root = 0; root < comm_size; root++)
        MPI_Gather (times + rank, sizeof (time_run_t), MPI_CHAR, times, sizeof (time_run_t), MPI_CHAR, root, MPI_COMM_WORLD);
    time_run_t max_time = 0;
    for (int i = 0; i < comm_size; i++) {
        if (times[i] > max_time)
            max_time = times[i];
    }
    free (times);
    return max_time;
}

double* gather_vertical_submatrix (double *a, const int i_size, const int j_size, const int j_for_rank)
{
    int rank = -1, comm_size = -1;
    MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    if (comm_size == 1) {
        return a;
    }
    if (!rank) {
        double *a_full = calloc (i_size * j_size, sizeof (double));
        for (int i = 0; i < i_size; i++)
            memcpy (a_full + i * j_size, a + i * j_for_rank, j_for_rank * sizeof (double));
        free (a);
        double *a_recv = calloc (((j_size - fetch_sz) / comm_size + fetch_sz) * i_size, sizeof (double));
        for (int sender = 1; sender < comm_size; sender++) {
            int j_for_sender = (j_size - fetch_sz) / comm_size;
            int int_num_rest = (j_size - fetch_sz) % comm_size;
            int shift = sender * j_for_sender;
            if (sender >= int_num_rest)
                shift = int_num_rest * (j_for_sender + 1) + (sender - int_num_rest) * j_for_sender;
            if (sender < int_num_rest)
                j_for_sender++;
            if (sender == comm_size - 1)
                j_for_sender += fetch_sz;
            MPI_Recv (a_recv, i_size * j_for_sender, MPI_DOUBLE, sender, MPI_TAG_GATHER, MPI_COMM_WORLD, NULL);
            for (int i = 0; i < i_size; i++)
                memcpy (a_full + i * j_size + shift, a_recv + i * j_for_sender, j_for_sender * sizeof (double));
        }
        free (a_recv);
        return (a_full);
    }
    else {
        MPI_Send (a, i_size * j_for_rank, MPI_DOUBLE, 0, MPI_TAG_GATHER, MPI_COMM_WORLD);
        return (a);
    }
    return NULL;
}
