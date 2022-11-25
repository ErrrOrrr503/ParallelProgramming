#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <math.h>

const char *single_file = NULL;

typedef double time_run_t;

int main (int argc, char **argv)
{
    if (argc != 4 && argc != 3) {
        printf ("usage: %s <i size> <j_size> <result file>", argv[0]);
        return -1;
    }
    int i_size = -1;
    int j_size = -1;
    if (argc == 4) {
        i_size = atoi (argv[2]);
        j_size = atoi (argv[3]);
        single_file = argv[1];
    }
    else {
        single_file = NULL;
        i_size = atoi (argv[1]);
        j_size = atoi (argv[2]);
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
     * In fact ordered is likely not to be used at all in our case.
    */
    time_run_t start = omp_get_wtime();
    for (int i = 0; i < i_size - 3; i++)
    {
        double *a_i = a + i * j_size;
        double *a_i__3 = a + (i + 3) * j_size;
        for (int j = 2; j < j_size; j++)
        {
            a_i[j] = sin (0.1 * a_i__3[j - 2]);
        }
    }
    time_run_t time = omp_get_wtime() - start;

    if (single_file != NULL) {
        FILE *ff = fopen(single_file,"w");
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