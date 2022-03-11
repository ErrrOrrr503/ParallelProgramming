#include <stdio.h>
#include <mpi.h>

int main (int argc, char* argv[])
{
    MPI_Init (&argc, &argv);
    // all the processes will further code
    int comm_size = -1, rank = -1;
    MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    printf ("Hello form rank '%d' of '%d'\n", rank, comm_size);
    MPI_Finalize ();
    return 0;
}