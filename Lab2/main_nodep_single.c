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
        printf ("usage: %s <result file> <i size> <j_size>", argv[0]);
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
    time_run_t start = omp_get_wtime();
    for (int i = 0; i < i_size; i++)
    {
        double *a_i = a + i * j_size;
        for (int j = 0; j < j_size; j++)
        {
            a_i[j] = sin (2 * a_i[j]);
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