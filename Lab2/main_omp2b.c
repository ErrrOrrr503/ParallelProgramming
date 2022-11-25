#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <math.h>

const char *omp_file = NULL;

typedef double time_run_t;

int main (int argc, char **argv)
{
    if (argc != 5 && argc != 4) {
        printf ("usage: %s <num_threads> <result file> <i size> <j_size>", argv[0]);
        return -1;
    }
    int i_size = -1;
    int j_size = -1;
    int num_threads = 0;
    if (argc == 5) {
        i_size = atoi (argv[3]);
        j_size = atoi (argv[4]);
        omp_file = argv[2];
        num_threads = atoi (argv[1]);
    }
    else {
        omp_file = NULL;
        i_size = atoi (argv[2]);
        j_size = atoi (argv[3]);
        num_threads = atoi (argv[1]);
    }
    double *a = (double *) calloc (i_size * j_size, sizeof (double));
    int i, j;
    //подготовительная часть – заполнение некими данными
    for (i = 0; i < i_size; i++) {
        for (j = 0; j < j_size; j++) {
            a[i * j_size + j] = 10 * i + j;
        }
    }
    // task
    /*
     * Lets analyze dependencies:
     * Linear system:
     * i = k + 3; j = l - 2
     * Distance vector is (-3, +2)
     * We have true dependecy by j and false dependency by i.
     * In fact after collapse we have one big false dependency. But
     * We can't afford copying for all threads.
     * We can't rely on "the dependency is long, one thread won't overrun other for the distance". Tell that LITTLEBIG with cortexa53 and X1 in one soc.
     * So we cant manually collapse as omp does not support variable dependency lengths.
     * Omp does not support false deps. But we can convert it into true one.
    */
    int chunk_size = (3 * j_size - 2) / num_threads; 
    time_run_t start = omp_get_wtime();
    #pragma omp parallel for shared (a) collapse (2) num_threads (num_threads) schedule (static, chunk_size) ordered (2)
    for (int i = 3; i < i_size; i++)
    {
        for (int j = 0; j < j_size - 2; j++)
        {
            #pragma omp ordered depend (sink:i-3,j+2)
            a[(i - 3) * j_size + j + 2] = sin (0.1 * a[i * j_size + j]);
            #pragma omp ordered depend (source)
        }
    }
    time_run_t time = omp_get_wtime() - start;

    if (omp_file != NULL) {
        FILE *ff = fopen(omp_file,"w");
        for(int i = 0; i < i_size; i++){
            for (int j = 0; j < j_size; j++) {
                fprintf(ff,"%f ",a[i * j_size + j]);
            }
            fprintf(ff,"\n");
        }
        fclose(ff);
    }
    free (a);

    printf ("%f\n", time);
    return 0;
}