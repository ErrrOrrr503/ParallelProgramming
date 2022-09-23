#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main (int argc, char *argv[])
{
    if (argc != 2) {
        printf ("usage: %s <N>\n", argv[0]);
        return 1;
    }
    long N = atoll (argv[1]);
    long double S = 0;
    #pragma omp parallel shared (N)
    {
        #pragma omp for reduction (+:S)
        for (long i = 1; i <= N; i++)
            S += 1 / (long double) i;
    }
    printf ("S = %llg\n", S);
    return 0;
}