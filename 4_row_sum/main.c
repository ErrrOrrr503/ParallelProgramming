#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <assert.h>
#include <inttypes.h>
#include <string.h>

// N - number of row members, an argument
// 0! is a special case, so we will count from 1-st member and just add 0-th (1 for e)
// for pi using Newton's methof of pi/2 = SUM k!/(2k + 1)!!

typedef long double fact_t; // uint64_t overflows too fast resulting in 'inf'
typedef long double float_t;

int main (int argc, char *argv[])
{
    MPI_Init (&argc, &argv);
    int rank = -1, comm_size = -1;
    MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    if (argc < 2 || (argc > 2 && (strcmp (argv[2], "-pi") && strcmp (argv[2], "-e")))) {
        if (!rank)
            printf ("usage: mpirun %s <N - number of row members> [-e(default) or -pi to calculate pi]\n", argv[0]);
        return 1;
    }
    char calc_pi = 0;
    if (argc == 3 && !strcmp (argv[2], "-pi"))
        calc_pi = 1;
    long long N = atoll (argv[1]);
    assert (N > 0);
    
    double time_start = MPI_Wtime ();
    // first processes receive N/nproc + 1, rest will receive N/nproc.
    // precalc and share factorials and double factorials
    // in fact factorials are pseudo factorials:
    // rank0: calculates 1 2 6 ... N!
    // rank1: (N+1) (N+1)(N+2) ... - pseudo factprials
    // ...
    // so real factorial for rank is max_rank_fact[rank ] * pseudo factorial
    long long N_nproc = N / comm_size;
    long long N_nproc_1 = N_nproc + 1;
    long long N_rest = N % comm_size;
    float_t S = 0;
    fact_t *max_rank_fact = (fact_t *) malloc ((comm_size + 1) * sizeof (fact_t)); // extra one for Gather workaround: rnak_k sends max_rank_fact [k+1] to k+1, not to k
    fact_t *max_rank_fact2 = NULL;
    max_rank_fact[0] = 1;
    if (calc_pi) {
        max_rank_fact2 = (fact_t *) malloc ((comm_size + 1) * sizeof (fact_t));
        max_rank_fact2[0] = 1;
    }
    fact_t *factorials = NULL;
    fact_t *factorials2 = NULL;
    if (rank < N_rest) {
        factorials = (fact_t *) malloc (N_nproc_1 * sizeof (fact_t));
        if (calc_pi)
            factorials2 = (fact_t *) malloc (N_nproc_1 * sizeof (fact_t));
        long long start = N_nproc_1 * rank + 1;
        for (long long num = start; (num - start) < N_nproc_1; num++) {
            if (num == start)
                factorials[num - start] = num;
            else
                factorials[num - start] = factorials[num - start - 1] * num;
            if (calc_pi) {
                if (num == start)
                    factorials2[num - start] = 2 * num + 1;
                else
                    factorials2[num - start] = factorials2[num - start - 1] * (2 * num + 1);
            }
        }
        if (rank < comm_size - 1) {
            max_rank_fact[rank + 1] = factorials[N_nproc_1 - 1];
            if (calc_pi)
                max_rank_fact2[rank + 1] = factorials2[N_nproc_1 - 1];
        }
    }
    // just same as above but with different N_proc.
    else {
        factorials = (fact_t *) malloc (N_nproc * sizeof (fact_t));
        if (calc_pi)
            factorials2 = (fact_t *) malloc (N_nproc * sizeof (fact_t));
        long long start = N_nproc_1 * N_rest + N_nproc * (rank - N_rest) + 1;
        for (long long num = start; (num - start) < N_nproc; num++) {
            if (num == start)
                factorials[num - start] = num;
            else
                factorials[num - start] = factorials[num - start - 1] * num;
            if (calc_pi) {
                if (num == start)
                    factorials2[num - start] = 2 * num + 1;
                else
                    factorials2[num - start] = factorials2[num - start - 1] * (2 * num + 1);
            }
        }
        if (rank < comm_size - 1) {
            max_rank_fact[rank + 1] = factorials[N_nproc - 1];
            if (calc_pi)
                max_rank_fact2[rank + 1] = factorials2[N_nproc - 1];
        }
    }
    // share all to all
    for (int i = 1; i < comm_size; i++)
        MPI_Gather (&max_rank_fact[rank + 1], sizeof (fact_t), MPI_CHAR, &max_rank_fact[1], sizeof (fact_t), MPI_CHAR, i, MPI_COMM_WORLD);
    if (calc_pi) {
        for (int i = 1; i < comm_size; i++)
            MPI_Gather (&max_rank_fact2[rank + 1], sizeof (fact_t), MPI_CHAR, &max_rank_fact2[1], sizeof (fact_t), MPI_CHAR, i, MPI_COMM_WORLD);
    }
    // finalize calculation for max_rank_fact
    for (int i = 2; i < comm_size; i++) {
        max_rank_fact[i] *= max_rank_fact[i - 1];
        if (calc_pi)
            max_rank_fact2[i] *= max_rank_fact2[i - 1];
    }
    //calculate sums
    if (rank < N_rest) {
        long long start = N_nproc_1 * rank + 1;
        for (long long num = start; (num - start) < N_nproc_1; num++) {
            if (!calc_pi)
                S += (float_t) 1 / (float_t) factorials[num - start] / (float_t) max_rank_fact[rank];
            if (calc_pi)
                S += (float_t) factorials[num - start] / (float_t) factorials2[num - start] / (float_t) max_rank_fact2[rank] * (float_t) max_rank_fact[rank];
        }
    }
    else {
        long long start = N_nproc_1 * N_rest + N_nproc * (rank - N_rest) + 1;
        for (long long num = start; (num - start) < N_nproc; num++) {
            if (!calc_pi)
                S += (float_t ) 1 / (float_t) factorials[num - start] / (float_t) max_rank_fact[rank];
            if (calc_pi)
                S += (float_t) factorials[num - start] / (float_t) factorials2[num - start] / (float_t) max_rank_fact2[rank] * (float_t) max_rank_fact[rank];
        }
    }
    MPI_Request unused_request;
    // reduce sums to one
    float_t S0 = 0;
    MPI_Reduce (&S, &S0, 1, MPI_LONG_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    double time_rank_end = MPI_Wtime ();
    if (rank) {
        printf ("Process %d finished in = %.10f seconds\n", rank, time_rank_end - time_start);
    }
    else {
        printf ("Main process finished in = %.10f seconds, waiting for others\n", time_rank_end - time_start);
        if (!calc_pi)
            printf ("e = %.18llg\n", S0 + 1);
        if (calc_pi)
            printf ("p = %.18llg\n", 2 * (S0 + 1));
    }
    free (max_rank_fact);
    free (factorials);
    if (calc_pi) {
        free (max_rank_fact2);
        free (factorials2);
    }
    MPI_Finalize ();
    return 0;
}