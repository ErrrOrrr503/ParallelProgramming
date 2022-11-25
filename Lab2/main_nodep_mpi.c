#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <math.h>

const char *mpi_file = NULL;

typedef double time_run_t;

#define MPI_TAG_EDGE 0
#define MPI_TAG_GATHER 1

int fetch_sz = 0;

time_run_t gather_max_time (time_run_t time);
void calculate_limits (int rank, int j_size, int *j_for_rank, int *int_num_rest, int *int_rank_shift);

int main (int argc, char **argv)
{
    MPI_Init (NULL, NULL);
    int rank = -1, comm_size = -1;
    MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    if (argc != 4 && argc != 3) {
        if (!rank)
            printf ("usage: mpirun -c <threads> %s <i size> <j_size> <result file>", argv[0]);
        return -1;
    }
    int i_size = -1;
    int j_size = -1;
    if (argc == 4) {
        i_size = atoi (argv[2]);
        j_size = atoi (argv[3]);
        mpi_file = argv[1];
    }
    else {
        mpi_file = NULL;
        i_size = atoi (argv[1]);
        j_size = atoi (argv[2]);
    }
    int size = j_size * i_size;
    int j_for_rank = -1, int_num_rest = -1, int_rank_shift = -1;
    calculate_limits (rank, size, &j_for_rank, &int_num_rest, &int_rank_shift);
    double *a = NULL;
    if (!rank)
        a = (double *) calloc (i_size * j_size, sizeof (double));
    else
        a = (double *) calloc (j_for_rank, sizeof (double));
    int *recvcounts = calloc (comm_size, sizeof (int));
    int *displs = calloc (comm_size, sizeof (int));
    for (int sender = 0; sender < comm_size; sender++) {
        int j_for_sender = -1, temp = -1, int_sender_shift = -1;
        calculate_limits (sender, size, &j_for_sender, &temp, &int_sender_shift);
        recvcounts[sender] = j_for_sender;
        displs[sender] = int_sender_shift;
    }
    //подготовительная часть – заполнение некими данными
    for (int k = 0; k < j_for_rank; k++) {
        int j = (int_rank_shift + k) % j_size;
        int i = (int_rank_shift + k) / j_size;
        a[k] = 10 * i + j;
    }
    // task
    time_run_t time_start = MPI_Wtime ();
    for (int k = 0; k < j_for_rank; k++)
    {
        a[k] = sin (2 * a[k]);
    }
    
    time_run_t time_calc = MPI_Wtime () - time_start;

    // gather time and figure out MAX time_calc.
    /*
    time_run_t max_time = gather_max_time (time_calc);
    if (!rank)
        printf ("%f\n", max_time);
    */
    //gather result
    MPI_Gatherv (a, j_for_rank, MPI_DOUBLE, a, recvcounts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    
    time_run_t time_full = MPI_Wtime () - time_start;

    // gather time and figure out MAX time_calc.
    time_run_t max_time = gather_max_time (time_full);
    if (!rank)
        printf ("%f\n", max_time);
    
    if (!rank) {
        if (mpi_file != NULL) {
            FILE *ff = fopen(mpi_file,"w");
            for (int i = 0; i < i_size; i++){
                for (int j = 0; j < j_size; j++) {
                    fprintf(ff,"%f ", a[i * j_size + j]);
                }
                fprintf(ff,"\n");
            }
            fclose(ff);
        }
    }
    free (a);

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

void calculate_limits (int rank, int j_size, int *j_for_rank, int *int_num_rest, int *int_rank_shift)
{
    int comm_size = -1;
    MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
    *j_for_rank = (j_size - fetch_sz) / comm_size;
    *int_num_rest = (j_size - fetch_sz) % comm_size;
    *int_rank_shift = rank * (*j_for_rank + 1);
    if (rank >= *int_num_rest)
        *int_rank_shift = *int_num_rest * (*j_for_rank + 1) + (rank - *int_num_rest) * *j_for_rank;
    if (rank < *int_num_rest)
        (*j_for_rank)++;
    if (rank == comm_size - 1)
        *j_for_rank += fetch_sz;
}