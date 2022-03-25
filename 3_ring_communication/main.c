#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <time.h>

// elements are variable length so 'array' is like [size_t offsets indexes[comm_size]][data] in one stream (not to pass 2 arrays separately)
// array elements can be specified as command line arguments.
// the number of elements equals to NP - number of processes.
// elements are treated as char arrays: strings or just raw data.
// data is loaded/generate by rank0 and then passed to others
// ring is reversed order (0 -> NP-1 -> NP-2 ... -> 1) to reduce checks (rank0 can send msg to last rank without checking, but must check if rank1 exists otherwise)

int main (int argc, char* argv[])
{
    MPI_Init (&argc, &argv);
    int rank = -1, comm_size = -1;
    MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    MPI_Request unused_request;
    char *data = NULL; // just raw byte
    size_t *offsets = NULL;
    size_t SZ = 0;
    if (!rank) {
        // prepare
        if (argc != comm_size + 1) {
            printf ("To specify array use: mpirun %s <array of len NP>\nArray len of %d is not equal to NP of %d\nUsing random array\n\n", argv[0], argc - 1, comm_size);
            SZ = (comm_size) * sizeof (size_t) + comm_size * 2 * sizeof (char);
            data = malloc (SZ);
            offsets = (size_t *) data;
            for (size_t i = 0; i < comm_size; i++) {
                offsets[i] = (comm_size) * sizeof (size_t) + i * 2; // set offset
                srand (clock ());
                *(data + offsets[i]) = (char) (rand () % 95 + '!'); // set elem
                *(data + offsets[i] + 1) = '\0';
            } 
        }
        else {
            size_t text_size = 0;
            for (size_t i = 0; i < argc - 1; i++)
                text_size += strlen (argv[i + 1]) + 1;
            SZ = (comm_size) * sizeof (size_t) + text_size;
            data = malloc (SZ);
            offsets = (size_t *) data;
            size_t processed_text_size = 0;
            for (size_t i = 0; i < comm_size; i++) {
                offsets[i] = (comm_size) * sizeof (size_t) + processed_text_size;
                strcpy ((data + offsets[i]), argv[i + 1]);
                processed_text_size += strlen (argv[i + 1]) + 1;
            }
        }
        
        printf ("data[%d] = '%s' offset %zu\n", rank, (data + offsets[rank]), offsets[rank]);
        MPI_Isend (&SZ, sizeof (char) * sizeof (size_t), MPI_CHAR, comm_size - 1, 1, MPI_COMM_WORLD, &unused_request);
        MPI_Isend (data, sizeof (char) * SZ, MPI_CHAR, comm_size - 1, 0, MPI_COMM_WORLD, &unused_request);
    }
    else {
        MPI_Recv (&SZ, sizeof (char) * sizeof (size_t), MPI_CHAR, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, NULL);
        //printf ("SZ = %zu\n", SZ);
        data = malloc (SZ);
        offsets = (size_t *) data;
        MPI_Recv (data, sizeof (char) * SZ, MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, NULL);
        printf ("data[%d] = '%s' offset %zu\n", rank, (data + offsets[rank]), offsets[rank]);
        MPI_Isend (&SZ, sizeof (char) * sizeof (size_t), MPI_CHAR, rank - 1, 1, MPI_COMM_WORLD, &unused_request);
        MPI_Isend (data, sizeof (char) * SZ, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD, &unused_request);
    }
    free (data);
    MPI_Finalize ();
    return 0;
}